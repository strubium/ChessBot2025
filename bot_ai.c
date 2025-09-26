#include "chessapi.h"
#include <limits.h>
#include <stdint.h>
#include <string.h>
#include <stdbool.h>

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
    for (int i = PAWN; i <= KING; i++) {
        BitBoard w_bb = chess_get_bitboard(board, WHITE, i);
        BitBoard b_bb = chess_get_bitboard(board, BLACK, i);
        score += piece_value(i) * (__builtin_popcountll(w_bb) - __builtin_popcountll(b_bb));
    }

    // Penalize repeated positions strongly
    uint64_t hash = chess_zobrist_key(board);
    int repeat_count = 0;
    for (int i = 0; i < history_len; i++)
        if (history[i] == hash)
            repeat_count++;
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
    Move best_move = moves[0];
    bool maximizing = chess_is_white_turn(board);
    int best_score = maximizing ? INT_MIN : INT_MAX;

    for (int i = 0; i < len; i++) {
        if (would_repeat(board, moves[i], history, history_len))
            continue;

        // Make move
        chess_make_move(board, moves[i]);
        uint64_t hash = chess_zobrist_key(board);
        history[history_len] = hash;

        int score = minimax(board, depth - 1, !maximizing, INT_MIN, INT_MAX, history, history_len + 1);

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

        Move best = find_best_move(board, 4, board_history, history_len); // depth 4 safe

        chess_push(best);
        board_history[history_len++] = chess_zobrist_key(board);

        chess_done();
        chess_free_board(board);
    }

    return 0;
}
