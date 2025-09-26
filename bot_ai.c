#include "chessapi.h"
#include <limits.h>
#include <stdint.h>
#include <string.h>

// --- Piece values ---
int piece_value(PieceType piece) {
    switch(piece) {
        case PAWN: return 100;
        case KNIGHT: return 320;
        case BISHOP: return 330;
        case ROOK: return 500;
        case QUEEN: return 900;
        default: return 10000; // king
    }
}

// --- Board evaluation (material only, optional: repetition penalty) ---
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

    score -= repeat_count * 500; // strong penalty to avoid cycles
    return score;
}

// --- Check if move would cause a threefold repetition ---
bool would_repeat(Board *board, Move m, uint64_t history[], int history_len) {
    chess_make_move(board, m);
    uint64_t hash = chess_zobrist_key(board);
    chess_undo_move(board);

    int count = 0;
    for (int i = 0; i < history_len; i++)
        if (history[i] == hash)
            count++;

    return count >= 2; // skip move if it would cause 3rd occurrence
}

// --- Minimax with repetition avoidance ---
int minimax(Board *board, int depth, bool maximizing, uint64_t history[], int history_len) {
    GameState state = chess_get_game_state(board);
    if (depth == 0 || state != GAME_NORMAL)
        return evaluate_board(board, history, history_len);

    int len;
    Move *moves = chess_get_legal_moves(board, &len);
    if (len == 0) {
        chess_free_moves_array(moves);
        return evaluate_board(board, history, history_len);
    }

    int best_score = maximizing ? INT_MIN : INT_MAX;

    for (int i = 0; i < len; i++) {
        if (would_repeat(board, moves[i], history, history_len))
            continue;

        int capture_bonus = 0;
        if (moves[i].capture) {
            PieceType captured = chess_get_piece_from_bitboard(board, moves[i].to);
            capture_bonus = piece_value(captured);
        }

        // Make move and add board hash to history
        chess_make_move(board, moves[i]);
        history[history_len++] = chess_zobrist_key(board);

        int score = minimax(board, depth - 1, !maximizing, history, history_len);
        chess_undo_move(board);
        history_len--; // remove hash after undo

        score += maximizing ? capture_bonus : -capture_bonus;

        if ((maximizing && score > best_score) || (!maximizing && score < best_score))
            best_score = score;
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

        int capture_bonus = 0;
        if (moves[i].capture) {
            PieceType captured = chess_get_piece_from_bitboard(board, moves[i].to);
            capture_bonus = piece_value(captured);
        }

        // Make move and add hash
        chess_make_move(board, moves[i]);
        history[history_len++] = chess_zobrist_key(board);

        int score = minimax(board, depth - 1, !maximizing, history, history_len);
        chess_undo_move(board);
        history_len--; // remove hash after undo

        score += maximizing ? capture_bonus : -capture_bonus;

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

        Move best = find_best_move(board, 2, board_history, history_len);

        // Push move and add hash to history
        chess_push(best);
        board_history[history_len++] = chess_zobrist_key(board);

        chess_done();
        chess_free_board(board);
    }

    return 0;
}
