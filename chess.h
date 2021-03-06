#pragma once

#include <stdbool.h>

typedef enum { PIECE_TYPE_PAWN, PIECE_TYPE_KNIGHT, PIECE_TYPE_BISHOP, PIECE_TYPE_ROOK, PIECE_TYPE_QUEEN, PIECE_TYPE_KING } piece_type;

struct square {
	bool has_piece;
	bool is_piece_white; // if has_piece, whether the piece color on this square is white
	piece_type piece_type;
};

struct move {
	piece_type piece_type;
	bool is_piece_white;
	int source_rank, source_file;
	int target_rank, target_file;
	bool is_capture;
	piece_type captured_piece_type;    // only applies if is_capture == true
	bool is_check;
	bool is_mate; 

	// following fields only apply if piece_type == PIECE_TYPE_PAWN
	bool is_promotion;		   		   
	bool is_en_passant;
	piece_type piece_type_promoted_to;
};


struct position {
	// [rank][file]
	struct square squares[8][8];

	int white_king_rank;
	int white_king_file;
	int black_king_rank;
	int black_king_file;
	
	bool white_can_castle_kingside;
	bool black_can_castle_kingside;
	bool white_can_castle_queenside;
	bool black_can_castle_queenside;

	bool can_en_passant[8]; // per file, regardless of color
};

#define GAME_ONGOING 0
#define WHITE_WON 1
#define BLACK_WON 2

struct game_state {
	struct position positions[256];
	int n_positions;
	
	struct position *current_position;
	
	bool white_to_move;
	
	int result;
	
	struct move moves[256];
	int n_moves;
};

// there up to 8 knight moves for a single knight
// each pair of values here are a potential knight move
static int knight_move_rank_offsets[8] = { 2, 1, -1, -2, -2, -1,  1,  2 };
static int knight_move_file_offsets[8] = { 1, 2,  2,  1, -1, -2, -2, -1 };


// a king can move in 8 directions, rank_offsets[i] and file_offsets[i] are one directional pair
static int king_move_rank_offsets[8] = { -1, -1, -1,  0,  1, 1, 1, 0 };
static int king_move_file_offsets[8] = {  1,  0, -1, -1, -1, 0, 1, 1 };


#define MOVE_IS_NOT_CHECK_OR_MATE 0
#define MOVE_IS_CHECK 1
#define MOVE_IS_MATE 2
int is_move_check_or_mate(struct position *position, struct move *move);

int find_all_possible_moves_for_piece(struct position *position, struct move *into, int rank, int file);

int find_all_possible_moves_for_color(struct position *position, struct move *into, bool color_is_white);

void apply_move_to_game_state(struct game_state *game_state, const struct move *the_move);