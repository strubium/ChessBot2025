#include "chessapi.h"
#include <limits.h>
#include <stdint.h>
#include <string.h>

// --- Piece values ---
int piece_value(PieceType piece) {
    switch(piece) {
        case PAWN:   return 100;
        case KNIGHT: return 320;
        case BISHOP: return 330;
        case ROOK:   return 500;
        case QUEEN:  return 900;
        default:     return 10000; // King
    }
}

// --- Board evaluation (material only, with repetition penalty) ---
int evaluate_board(Board *board, uint64_t history[], int history_len) {
    int score = 0;

    // --- Material evaluation ---
    for (int piece = PAWN; piece <= KING; piece++) {
        BitBoard w_bb = chess_get_bitboard(board, WHITE, piece);
        BitBoard b_bb = chess_get_bitboard(board, BLACK, piece);
        score += piece_value(piece) * (__builtin_popcountll(w_bb) - __builtin_popcountll(b_bb));
    }

    // --- Development bonus: knights and bishops off starting squares ---
    BitBoard w_knight = chess_get_bitboard(board, WHITE, KNIGHT);
    BitBoard w_bishop = chess_get_bitboard(board, WHITE, BISHOP);
    BitBoard b_knight = chess_get_bitboard(board, BLACK, KNIGHT);
    BitBoard b_bishop = chess_get_bitboard(board, BLACK, BISHOP);

    BitBoard w_dev = (w_knight & ~((1ULL << 1) | (1ULL << 6))) |
                 (w_bishop & ~((1ULL << 2) | (1ULL << 5)));

    BitBoard b_dev = (b_knight & ~((1ULL << 57) | (1ULL << 62))) |
                     (b_bishop & ~((1ULL << 58) | (1ULL << 61)));

    score += 15 * (__builtin_popcountll(w_dev) - __builtin_popcountll(b_dev));

    // --- Center control bonus ---
    BitBoard center = 1ULL << 27 | 1ULL << 28 | 1ULL << 35 | 1ULL << 36;

    BitBoard w_center = (chess_get_bitboard(board, WHITE, PAWN) |
                         chess_get_bitboard(board, WHITE, KNIGHT) |
                         chess_get_bitboard(board, WHITE, BISHOP) |
                         chess_get_bitboard(board, WHITE, QUEEN)) & center;

    BitBoard b_center = (chess_get_bitboard(board, BLACK, PAWN) |
                         chess_get_bitboard(board, BLACK, KNIGHT) |
                         chess_get_bitboard(board, BLACK, BISHOP) |
                         chess_get_bitboard(board, BLACK, QUEEN)) & center;

    score += 25 * (__builtin_popcountll(w_center) - __builtin_popcountll(b_center));

    // --- King safety / castling bonus ---
    if (chess_can_kingside_castle(board, WHITE) || chess_can_queenside_castle(board, WHITE))
        score += 40;
    if (chess_can_kingside_castle(board, BLACK) || chess_can_queenside_castle(board, BLACK))
        score -= 40;

    // --- Penalize repeated positions ---
    uint64_t hash = chess_zobrist_key(board);
    int repeat_count = 0;
    for (int i = 0; i < history_len; i++)
        if (history[i] == hash) repeat_count++;

    score -= repeat_count * 500;

    return score;
}

// --- Check if move would cause a threefold repetition ---
bool would_repeat(Board *board, Move m, uint64_t history[], int history_len) {
    // Include the current board as the first occurrence
    uint64_t current_hash = chess_zobrist_key(board);

    chess_make_move(board, m);
    uint64_t new_hash = chess_zobrist_key(board);
    chess_undo_move(board);

    int count = 0;
    // count occurrences in history
    for (int i = 0; i < history_len; i++)
        if (history[i] == new_hash)
            count++;

    // include current board hash as part of the path
    if (new_hash == current_hash)
        count++;

    return count >= 3;
}


// --- Minimax with alpha-beta pruning and proper history handling ---
int minimax(Board *board, int depth, bool maximizing, int alpha, int beta,
            uint64_t path[], int path_len) {

    GameState state = chess_get_game_state(board);
    if (depth == 0 || state != GAME_NORMAL)
        return evaluate_board(board, path, path_len);

    int len;
    Move *moves = chess_get_legal_moves(board, &len);
    if (len == 0) {
        chess_free_moves_array(moves);
        return evaluate_board(board, path, path_len);
    }

    int best_score = maximizing ? INT_MIN : INT_MAX;

    for (int i = 0; i < len; i++) {
        // Temporarily make move
        chess_make_move(board, moves[i]);
        uint64_t hash = chess_zobrist_key(board);

        // Check repetition along **this search path**
        int count = 0;
        for (int j = 0; j < path_len; j++)
            if (path[j] == hash)
                count++;
        if (count >= 2) { // 3rd occurrence = illegal
            chess_undo_move(board);
            continue;
        }

        // Append hash to path for recursion
        path[path_len] = hash;
        int score = minimax(board, depth - 1, !maximizing, alpha, beta, path, path_len + 1);

        chess_undo_move(board);

        if (maximizing) {
            if (score > best_score) best_score = score;
            if (score > alpha) alpha = score;
        } else {
            if (score < best_score) best_score = score;
            if (score < beta) beta = score;
        }

        if (beta <= alpha)
            break;
    }

    chess_free_moves_array(moves);
    return best_score;
}


// --- Find best move avoiding repetition ---
Move find_best_move(Board *board, int depth, uint64_t history[], int history_len) {
    int len;
    Move *moves = chess_get_legal_moves(board, &len);

    if (len == 0) {
        chess_free_moves_array(moves);
        return (Move){0}; // no moves
    }

    Move best_move = moves[0];
    bool maximizing = chess_is_white_turn(board);
    int best_score = maximizing ? INT_MIN : INT_MAX;

    // Temporary array for search path
    uint64_t path[1024];

    // Copy current history into path
    memcpy(path, history, history_len * sizeof(uint64_t));
    int path_len = history_len;

    for (int i = 0; i < len; i++) {
        // Skip moves that would cause repetition
        if (would_repeat(board, moves[i], history, history_len))
            continue;

        // Make move temporarily
        chess_make_move(board, moves[i]);
        uint64_t hash = chess_zobrist_key(board);

        // Add to path for recursion
        path[path_len] = hash;
        int score = minimax(board, depth - 1, !maximizing, INT_MIN, INT_MAX, path, path_len + 1);

        chess_undo_move(board);

        if ((maximizing && score > best_score) || (!maximizing && score < best_score)) {
            best_score = score;
            best_move = moves[i];
        }
    }

    chess_free_moves_array(moves);
    return best_move;
}

int main(void) {
    uint64_t board_history[1024]; // large enough buffer
    int history_len = 0;

    for (int i = 0; i < 500; i++) {
        Board *board = chess_get_board();
        if (chess_get_game_state(board) != GAME_NORMAL) {
            chess_free_board(board);
            break;
        }

        Move best = find_best_move(board, 5, board_history, history_len); // depth 5 safe

        chess_push(best);
        board_history[history_len++] = chess_zobrist_key(board);

        chess_done();
        chess_free_board(board);
    }

    return 0;
}
