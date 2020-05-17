#include <assert.h>
#include <stdio.h>
#include <stdint.h>
#include <inttypes.h>
#include <stdlib.h>
#include <string.h>
#include <stdbool.h>

#include "chess.h"
#include "chess_utils.h"

static int int_difference(int a, int b) {
	int signed_diff = a - b;
	if (signed_diff < 0)
		return -signed_diff;
	return signed_diff;
}


static void get_king_position(const struct position *position, bool is_king_white, int *rank, int *file) {
	if (is_king_white) {
		*rank = position->white_king_rank;
		*file = position->white_king_file;
	} else {
		*rank = position->black_king_rank;
		*file = position->black_king_file;
	}
}

void modify_squares_for_castled_rook(struct square *source_sq, struct square *target_sq, bool is_rook_white) {
	source_sq->has_piece = false;

	target_sq->has_piece = true;
	target_sq->piece_type = PIECE_TYPE_ROOK;
	target_sq->is_piece_white = is_rook_white;
}

static struct position saved_position_states[256];
static int n_saved_position_states = 0;

// applies move to position without any constraints, the move should be legal if the state of the position intends to be correct after
void apply_move_to_position(struct position *position, const struct move *move) {
	saved_position_states[n_saved_position_states] = *position;
	n_saved_position_states++;
	
	assert(move->source_rank >= 0);
	assert(move->source_rank <= 7);
	assert(move->source_file >= 0);
	assert(move->source_file <= 7);
	assert(move->target_rank >= 0);
	assert(move->target_rank <= 7);
	assert(move->target_file >= 0);
	assert(move->target_file <= 7);
	
	assert(position->squares[move->source_rank][move->source_file].has_piece);
	assert(position->squares[move->source_rank][move->source_file].piece_type == move->piece_type);

	// check for whether the move is a castle, since the rook castled with needs to move here
	// this only moves the rook that is being castled with, the king's move is taken care of by the general piece move code below
	if (move->piece_type == PIECE_TYPE_KING) {
		if (move->is_piece_white) {
			position->white_king_rank = move->target_rank;
			position->white_king_file = move->target_file;

			// if the source square is the king's starting square && target square is a castled position
			// AND the king can castle in that direction
			// then we move both the king and rook to the appropriate squares
			// e.g. king move from e1 to g1 means we move rook from h1 to f1 as well
			if (move->source_rank == 0 && move->source_file == 4) {
				
				if (move->target_rank == 0 && move->target_file == 6) { // white kingside castles
					struct square *h1_square = &position->squares[0][7];
					struct square *f1_square = &position->squares[0][5];

					modify_squares_for_castled_rook(h1_square, f1_square, move->is_piece_white);

				} else if (move->target_rank == 0 && move->target_file == 2) { // white queenside castles
					struct square *a1_square = &position->squares[0][0];
					struct square *d1_square = &position->squares[0][3];

					modify_squares_for_castled_rook(a1_square, d1_square, move->is_piece_white);
				}
			}

			// the moving side's king loses castling rights, regardless of the type of king move made here
			position->white_can_castle_kingside = false;
			position->white_can_castle_queenside = false;
		} else {
			position->black_king_rank = move->target_rank;
			position->black_king_file = move->target_file;

			// if king is moving from it's starting square of e8
			if (move->source_rank == 7 && move->source_file == 4) {
				if (move->target_rank == 7 && move->target_file == 6) { // white kingside castles
					struct square *h8_square = &position->squares[7][7];
					struct square *f8_square = &position->squares[7][5];

					modify_squares_for_castled_rook(h8_square, f8_square, move->is_piece_white);

				} else if (move->target_rank == 7 && move->target_file == 2) { // white queenside castles
					struct square *a8_square = &position->squares[7][0];
					struct square *d8_square = &position->squares[7][3];

					modify_squares_for_castled_rook(a8_square, d8_square, move->is_piece_white);
				}
			}

			// the moving side's king loses castling rights, regardless of the type of king move made here
			position->black_can_castle_kingside = false;
			position->black_can_castle_queenside = false;
		}

	} else if (move->piece_type == PIECE_TYPE_ROOK) {

		// a rook move that originates from the rook's original square (i.e. a1, h1, a8, h8)
		// causes the appropriate castling rights to be revoked
		if (move->source_rank == 0) {
			if (move->source_file == 0)
				position->white_can_castle_queenside = false;
			else if (move->source_file == 7)
				position->white_can_castle_kingside = false;

		} else if (move->source_rank == 7) {
			if (move->source_file == 0)
				position->black_can_castle_queenside = false;
			else if (move->source_file == 7)
				position->black_can_castle_kingside = false;
		}
		
	} 
	
	struct square *source_square = &position->squares[move->source_rank][move->source_file];
	struct square *target_square = &position->squares[move->target_rank][move->target_file];
	
	if (move->is_capture) {
		if (move->piece_type == PIECE_TYPE_PAWN && move->is_en_passant) {
			fprintf(stderr, "en passant move is %s\n", move_str(move));
			struct square *en_passanted_square = &position->squares[move->target_rank-1][move->target_file];
			assert(en_passanted_square->has_piece);
			assert(en_passanted_square->piece_type == PIECE_TYPE_PAWN);
			en_passanted_square->has_piece = false;
		} else {
			assert(target_square->has_piece);
		}
	}

	source_square->has_piece = false;

	target_square->has_piece = true;
	target_square->is_piece_white = move->is_piece_white;

	if (move->is_promotion) {
		target_square->piece_type = move->piece_type_promoted_to;
	} else {
		target_square->piece_type = move->piece_type;
	}

	// all previous en passant possibilities are gone after a move is made, there can only be one possibility on the next move
	// that's only if the current move is a pawn move 2 squares forward
	memset(position->can_en_passant, false, 8 * sizeof(position->can_en_passant[0])); 
	if (move->piece_type == PIECE_TYPE_PAWN) {
		if (int_difference(move->source_rank, move->target_rank) == 2) {
			//fprintf(stderr, "setting file %d can_en_passant to true\n", move->target_file);
			position->can_en_passant[move->target_file] = true;
		}
	}
}

void apply_move_to_game_state(struct game_state *game_state, const struct move *the_move) {
	struct position *new_position = &game_state->positions[game_state->n_positions];
	game_state->n_positions++;
	
	*new_position = *game_state->current_position;
	
	apply_move_to_position(new_position, the_move);
	
	if (the_move->is_mate) {
		if (the_move->is_piece_white)
			game_state->result = WHITE_WON;
		else
			game_state->result = BLACK_WON;
	}
	game_state->current_position = new_position;
	
	game_state->moves[game_state->n_moves] = *the_move;
	game_state->n_moves++;
	
	game_state->white_to_move = !the_move->is_piece_white;
	
	fprintf(stderr, "%s move: %s\n", the_move->is_piece_white ? "white's" : "black's", move_str(the_move));
}



static void undo_move_from_position(struct position *position, const struct move *move) {
	n_saved_position_states--;

	if (n_saved_position_states < 0) {
		fprintf(stderr, "n_saved_states is < 0 during undo_move_from_position\n");
		exit(1);
	}
	*position = saved_position_states[n_saved_position_states];
}

// returns whether the square [rank][file] is directly attacked by a piece of a color (white if is_color_white, otherwise black);
// from the provided direction
// the rank_dir and file_dir should both be in [-1, 1], otherwise the result doesn't really make sense
bool is_square_attacked_from_direction_by_color(const struct position *position, bool is_color_white, int rank, int file, int rank_dir, int file_dir) {
	#define DIAGONAL_DIRECTION 0
	#define STRAIGHT_DIRECTION 1

	// are we looking along a diagonal or a file/rank?
	int direction_we_are_checking;
	if (rank_dir == 0) {
		assert(file_dir != 0); // we should not call this function with rank_dir AND file_dir == 0, it'll spin forever
		direction_we_are_checking = STRAIGHT_DIRECTION;
	} else if (file_dir == 0) {
		assert(rank_dir != 0); // we should not call this function with rank_dir AND file_dir == 0, it'll spin forever
		direction_we_are_checking = STRAIGHT_DIRECTION;
	} else {
		direction_we_are_checking = DIAGONAL_DIRECTION;
	}

	// these are the coordinates of the square we are checking right now in the rank and file directions
	int target_rank = rank + rank_dir;
	int target_file = file + file_dir;

	int steps_in_direction = 1;

	// loop while we are within bounds of the position, if we exit bounds, then the square is not under attack from the direction
	while (target_rank >= 0 && target_rank <= 7 && target_file >= 0 && target_file <= 7) {		
		struct square target_square = position->squares[target_rank][target_file];

		if (target_square.has_piece) {
			bool is_target_square_piece_white = target_square.is_piece_white;
			
			if (is_target_square_piece_white != is_color_white) {
				// we've hit a piece of the opposite color in this direction
				// therefore the square is not attacked from this direction
				return false;
			
			} else if (target_square.piece_type == PIECE_TYPE_KING) {
				// if we've only made one step in the direction, and a king of the color is there, then the square is attacked
				if (steps_in_direction == 1) {
					return true;
				}

			} else if (target_square.piece_type == PIECE_TYPE_PAWN) {
				// if we've only made one step in the direction and checking diagonally
				// and a pawn is on the next rank, then the pawn is attacking that square
				if (steps_in_direction == 1 && direction_we_are_checking == DIAGONAL_DIRECTION) {
					if (is_target_square_piece_white) {
						if (rank_dir == -1)
							return true;
					} else {
						if (rank_dir == 1)
							return true;
					}
				}

			} else if (target_square.piece_type == PIECE_TYPE_QUEEN) {
				// the piece in this direction is a queen, which is checking the king regardless of whether our direction is diagonal or straight down a rank or file
				return true;
		
			} else if (target_square.piece_type == PIECE_TYPE_ROOK) {
				if (direction_we_are_checking == STRAIGHT_DIRECTION)
					return true;
				else
					return false;

			} else if (target_square.piece_type == PIECE_TYPE_BISHOP) {
				if (direction_we_are_checking == DIAGONAL_DIRECTION)
					return true;
				else
					return false;
			}
		}
		target_rank += rank_dir; target_file += file_dir;
		steps_in_direction++;
	}

	// we exited the loop without hitting a piece of any color, we are not in attack from this direction
	return false;
}

// returns whether square [rank][file] is attacked by a piece of a provided color
bool is_square_attacked_by_piece_of_color(const struct position *position, int rank, int file, bool is_color_white) {
	assert(rank >= 0);
	assert(rank <= 7);
	assert(file >= 0);
	assert(rank <= 7);

	// diagonal checks for bishop or queen attack, also includes attacks by pawn & king, which can only apply after 1 step in the direction
	{
		// -1, -1 direction
		if (is_square_attacked_from_direction_by_color(position, is_color_white, rank, file, -1, -1))
			return true;

		// -1, 1
		if (is_square_attacked_from_direction_by_color(position, is_color_white, rank, file, -1, 1))
			return true;

		// 1, -1 direction
		if (is_square_attacked_from_direction_by_color(position, is_color_white, rank, file, 1, -1))
			return true;

		// 1, 1 direction
		if (is_square_attacked_from_direction_by_color(position, is_color_white, rank, file, 1, 1))
			return true;
	}

	// attacks along a rank or file by either a queen or rook, also includes attacks by a king, which can only apply after 1 step in the direction
	{
		// 1, 0 direction
		if (is_square_attacked_from_direction_by_color(position, is_color_white, rank, file, 1, 0))
			return true;

		// -1, 0 direction
		if (is_square_attacked_from_direction_by_color(position, is_color_white, rank, file, -1, 0))
			return true;

		// 0, 1 direction
		if (is_square_attacked_from_direction_by_color(position, is_color_white, rank, file, 0, 1))
			return true;

		// 0, -1 direction
		if (is_square_attacked_from_direction_by_color(position, is_color_white, rank, file, 0, -1))
			return true;
	}

	// attacks by a knight
	{
		for (int i = 0; i < 8; i++) {
			int knight_rank = rank + knight_move_rank_offsets[i];
			int knight_file = file + knight_move_file_offsets[i];
	
			if (knight_rank < 0 || knight_rank > 7 || knight_file < 0 || knight_file > 7)
				continue;

			struct square the_square = position->squares[knight_rank][knight_file];
			
			if (the_square.has_piece && the_square.is_piece_white == is_color_white && the_square.piece_type == PIECE_TYPE_KNIGHT)
				return true;
		}
	}

	return false;
}

// returns whether the king located at king_rank, king_file is in check on the provided position
bool is_king_on_square_in_check(const struct position *position, int king_rank, int king_file) {
	if (!position->squares[king_rank][king_file].has_piece) {
		fprintf(stderr, "is_king_on_square_in_check target square %d %d does not have a piece at all!\n", king_rank, king_file);
		fprintf(stderr, "%s\n", position_str(position));
		exit(1);
	}
	if (position->squares[king_rank][king_file].piece_type != PIECE_TYPE_KING) {
		fprintf(stderr, "is_king_on_square_in_check target square %d %d does not have a king, it has piece %d!\n", king_rank, king_file, position->squares[king_rank][king_file].piece_type);
		fprintf(stderr, "%s\n", position_str(position));
		exit(1);
	}
	
	bool is_king_white = position->squares[king_rank][king_file].is_piece_white;

	// just check if the king's square is attacked by a piece of the opposite color
	return is_square_attacked_by_piece_of_color(position, king_rank, king_file, !is_king_white);
}

bool is_move_legal(struct position *position, struct move *move) {
	apply_move_to_position(position, move);

	int king_rank, king_file;
	get_king_position(position, move->is_piece_white, &king_rank, &king_file);

	// a move is illegal if it puts the mover's side's king in check
	bool is_illegal = is_king_on_square_in_check(position, king_rank, king_file);
	
	undo_move_from_position(position, move);

	return !is_illegal;
}

// does the following steps:
// checks if the move is legal
// if it is, checks if it the move is a check or mate
// if *intop is not NULL, records the move there, increments *intop, and increments *n_moves
void finalize_move_info_and_record_if_legal(struct position *position, struct move *move, struct move **intop, int *n_moves) {
	struct move *into = *intop;
	move->is_check = false;
	move->is_mate = false;
	if (is_move_legal(position, move)) {
		if (into != NULL) {
			int move_result = is_move_check_or_mate(position, move);
			if (move_result == MOVE_IS_MATE) {
				move->is_mate = true;
			} else if (move_result == MOVE_IS_CHECK) {
				move->is_check = true;
			}
			
			*into = *move;
			(*intop)++;
		}
		(*n_moves)++;
	}
}


int find_all_possible_pawn_moves(struct position *position, struct move **into, int rank, int file) {
	assert(rank >= 0 && rank <= 7);
	assert(file >= 0 && file <= 7);
	assert(position->squares[rank][file].has_piece);
	assert(position->squares[rank][file].piece_type == PIECE_TYPE_PAWN);

	bool is_pawn_white = position->squares[rank][file].is_piece_white;

	// next_rank for a pawn move of 1 square forward
	int next_rank;
	if (is_pawn_white) {
		next_rank = rank + 1;
	} else {
		next_rank = rank - 1;
	}
	// a pawn cannot end up on the last rank without promotion, so the square in front of a pawn must not be out of bounds
	assert(next_rank >= 0);
	assert(next_rank <= 7);
	
	int n_moves = 0;

	struct move next_move = {0};
	next_move.source_rank = rank;
	next_move.source_file = file;
	next_move.piece_type = PIECE_TYPE_PAWN;
	next_move.is_piece_white = is_pawn_white;

	piece_type possible_promotions[4] = { PIECE_TYPE_QUEEN, PIECE_TYPE_ROOK, PIECE_TYPE_BISHOP, PIECE_TYPE_KNIGHT };

	// move forward one square logic
	{
		struct square square_in_front_of_pawn = position->squares[next_rank][file];
		if (!square_in_front_of_pawn.has_piece) {
			next_move.target_rank = next_rank;
			next_move.target_file = file;
			next_move.is_capture = false;
			next_move.is_en_passant = false;

			// promotion case for pawn
			if ((next_rank == 0) || (next_rank == 7)) {
				next_move.is_promotion = true;

				for (int i = 0; i < 4; i++) {
					next_move.piece_type_promoted_to = possible_promotions[i];

					finalize_move_info_and_record_if_legal(position, &next_move, into, &n_moves);
				}
			} else { // forward move without promotion
				next_move.is_promotion = false;
				finalize_move_info_and_record_if_legal(position, &next_move, into, &n_moves);
			}

		}
	}
	

	// capture diagonally to the left
	{
		int left_file;
		if (is_pawn_white)
			left_file = file - 1;
		else
			left_file = file + 1;
		
		
		if (left_file >= 0 && left_file <= 7) {
			struct square target_square = position->squares[next_rank][left_file];
		
			if (target_square.has_piece && (target_square.is_piece_white != is_pawn_white)) {
				// we should never end up in situation where the target square has a king and the pawn can capture it
				assert(target_square.piece_type != PIECE_TYPE_KING);

				next_move.is_capture = true;
				next_move.target_rank = next_rank;
				next_move.target_file = left_file;
				next_move.captured_piece_type = target_square.piece_type;
				next_move.is_en_passant = false;

				if ((next_rank == 0) || (next_rank == 7)) {
					next_move.is_promotion = true;

					for (int i = 0; i < 4; i++) {
						next_move.piece_type_promoted_to = possible_promotions[i];
						finalize_move_info_and_record_if_legal(position, &next_move, into, &n_moves);
					}

				} else {
					next_move.is_promotion = false;
					finalize_move_info_and_record_if_legal(position, &next_move, into, &n_moves);
				}	
			}
		}
	}

	// capture diagonally to the right
	{
		int right_file;
		if (is_pawn_white)
			right_file = file + 1;
		else
			right_file = file - 1;

		if (right_file >= 0 && right_file <= 7) {
			struct square target_square = position->squares[next_rank][right_file];
		
			if (target_square.has_piece && (target_square.is_piece_white != is_pawn_white)) {
				// we should never end up in situation where the target square has a king of the opposite color and the pawn can capture it
				assert(target_square.piece_type != PIECE_TYPE_KING);

				next_move.is_capture = true;
				next_move.target_rank = next_rank;
				next_move.target_file = right_file;
				next_move.captured_piece_type = target_square.piece_type;
				next_move.is_en_passant = false;

				if ((next_rank == 0) || (next_rank == 7)) {
					next_move.is_promotion = true;

					for (int i = 0; i < 4; i++) {
						next_move.piece_type_promoted_to = possible_promotions[i];
						finalize_move_info_and_record_if_legal(position, &next_move, into, &n_moves);
					}

				} else {
					next_move.is_promotion = false;
					finalize_move_info_and_record_if_legal(position, &next_move, into, &n_moves);
				}	
			}
		}
	}

	// forward 2 squares logic
	{
		int target_rank = -1;
		if (is_pawn_white && rank == 1) {
			target_rank = rank + 2;
		} else if (!is_pawn_white && rank == 6) {
			target_rank = rank - 2;
		}

		if ((target_rank != -1) && !position->squares[target_rank][file].has_piece && !position->squares[next_rank][file].has_piece) {
			next_move.is_capture = false;
			next_move.is_promotion = false;
			next_move.target_rank = target_rank;
			next_move.target_file = file;
			next_move.is_en_passant = false;

			finalize_move_info_and_record_if_legal(position, &next_move, into, &n_moves);
		}
		
	}

	// en passant to the left of the pawn
	{
		int left_file;
		if (is_pawn_white)
			left_file = file - 1;
		else
			left_file = file + 1;
		
		if (left_file >= 0 && left_file <= 7) {
			
			struct square target_square = position->squares[rank][left_file];

			bool can_en_passant = target_square.has_piece && (target_square.is_piece_white != is_pawn_white) && (target_square.piece_type == PIECE_TYPE_PAWN) && position->can_en_passant[left_file];
			if (can_en_passant) {
				next_move.is_capture = true;
				next_move.captured_piece_type = PIECE_TYPE_PAWN;
				next_move.target_rank = next_rank;
				next_move.target_file = left_file;
				next_move.is_promotion = false;
				next_move.is_en_passant = true;

				finalize_move_info_and_record_if_legal(position, &next_move, into, &n_moves);
			}
		}
	}

	// en passant to the right of the pawn
	{
		int right_file;
		if (is_pawn_white)
			right_file = file + 1;
		else
			right_file = file - 1;
		
		if (right_file >= 0 && right_file <= 7) {
			
			struct square target_square = position->squares[rank][right_file];

			bool can_en_passant = target_square.has_piece && (target_square.is_piece_white != is_pawn_white) && (target_square.piece_type == PIECE_TYPE_PAWN) && position->can_en_passant[right_file];
			if (can_en_passant) {
				next_move.is_capture = true;
				next_move.captured_piece_type = PIECE_TYPE_PAWN;
				next_move.target_rank = next_rank;
				next_move.target_file = right_file;
				next_move.is_promotion = false;
				next_move.is_en_passant = true;

				finalize_move_info_and_record_if_legal(position, &next_move, into, &n_moves);
			}
		}
	}

	return n_moves;
}



int find_all_possible_knight_moves(struct position *position, struct move **into, int rank, int file) {
	assert(rank >= 0 && rank <= 7);
	assert(file >= 0 && rank <= 7);
	assert(position->squares[rank][file].has_piece);
	assert(position->squares[rank][file].piece_type == PIECE_TYPE_KNIGHT);

	bool is_knight_white = position->squares[rank][file].is_piece_white;

	struct move next_move = {0};
	next_move.piece_type = PIECE_TYPE_KNIGHT;
	next_move.is_piece_white = is_knight_white;
	next_move.source_rank = rank;
	next_move.source_file = file;
	
	int n_moves = 0;
	
	for (int i = 0; i < 8; i++) {
		int target_rank = rank + knight_move_rank_offsets[i];
		int target_file = file + knight_move_file_offsets[i];
	
		if (target_rank >= 0 && target_rank <= 7 && target_file >= 0 && target_file <= 7) {
		
			struct square target_square = position->squares[target_rank][target_file];
			
			next_move.target_rank = target_rank;
			next_move.target_file = target_file;
			
			if (target_square.has_piece) {
				if (target_square.is_piece_white == is_knight_white) {
					continue;
				}
		
				// we should never end up in situation where the target square has a king and the knight can capture it
				assert(target_square.piece_type != PIECE_TYPE_KING);
				
				next_move.is_capture = true;
				next_move.captured_piece_type = target_square.piece_type;
			} else {
				next_move.is_capture = false;
			}

			finalize_move_info_and_record_if_legal(position, &next_move, into, &n_moves);
		}
	}

	return n_moves;
}

int find_all_possible_moves_in_direction(struct position *position, struct move **into, int rank, int file, int rank_dir, int file_dir) {
	assert(rank >= 0 && rank <= 7);
	assert(file >= 0 && rank <= 7);
	assert(position->squares[rank][file].has_piece);

	piece_type moved_piece_type = position->squares[rank][file].piece_type;
	bool is_moved_piece_white = position->squares[rank][file].is_piece_white;

	struct move next_move = {0};
	next_move.piece_type = moved_piece_type;
	next_move.is_piece_white = is_moved_piece_white;
	next_move.source_rank = rank;
	next_move.source_file = file;
	
	int target_rank = rank + rank_dir;
	int target_file = file + file_dir;

	int n_moves = 0;

	while (1) {
		if (target_rank < 0 || target_rank > 7 || target_file < 0 || target_file > 7) // we went out of bounds
			break;
	
		struct square target_square = position->squares[target_rank][target_file];
			
		next_move.target_rank = target_rank;
		next_move.target_file = target_file;

		if (target_square.has_piece) {
			if (target_square.is_piece_white != is_moved_piece_white) {
				// we should never end up in situation where the target square has a king, this is an invalid state
				if (target_square.piece_type == PIECE_TYPE_KING) {
					fprintf(stderr, "find_all_possible_moves_in_direction target square has a king of the opposite color, target square: [%d, %d], piece square: [%d, %d], piece: %d\n", 
						target_rank, target_file, rank, file, moved_piece_type);
					exit(1);
				}

				next_move.is_capture = true;
				next_move.captured_piece_type = target_square.piece_type;
				finalize_move_info_and_record_if_legal(position, &next_move, into, &n_moves);
			}
			break;
		} else {
			next_move.is_capture = false;
			finalize_move_info_and_record_if_legal(position, &next_move, into, &n_moves);
		}

		target_rank += rank_dir;
		target_file += file_dir;
	}

	return n_moves;
}

int find_all_possible_bishop_moves(struct position *position, struct move **into, int rank, int file) {
	assert(rank >= 0 && rank <= 7);
	assert(file >= 0 && rank <= 7);
	assert(position->squares[rank][file].has_piece);
	assert(position->squares[rank][file].piece_type == PIECE_TYPE_BISHOP);

	int n_moves = 0;

	// +1, +1 diagonal direction
	int moves_in_direction = find_all_possible_moves_in_direction(position, into, rank, file, 1, 1);
	n_moves += moves_in_direction;

	// -1, +1 diagonal direction
	moves_in_direction = find_all_possible_moves_in_direction(position, into, rank, file, -1, 1);
	n_moves += moves_in_direction;

	// -1, -1 diagonal direction
	moves_in_direction = find_all_possible_moves_in_direction(position, into, rank, file, -1, -1);
	n_moves += moves_in_direction;

	// +1, -1 diagonal direction
	moves_in_direction = find_all_possible_moves_in_direction(position, into, rank, file, 1, -1);
	n_moves += moves_in_direction;

	return n_moves;
}


int find_all_possible_rook_moves(struct position *position, struct move **into, int rank, int file) {
	assert(rank >= 0 && rank <= 7);
	assert(file >= 0 && rank <= 7);
	assert(position->squares[rank][file].has_piece);
	assert(position->squares[rank][file].piece_type == PIECE_TYPE_ROOK);

	int n_moves = 0;

	int moves_in_direction = find_all_possible_moves_in_direction(position, into, rank, file, 1, 0);
	n_moves += moves_in_direction;

	moves_in_direction = find_all_possible_moves_in_direction(position, into, rank, file, -1, 0);
	n_moves += moves_in_direction;
	
	moves_in_direction = find_all_possible_moves_in_direction(position, into, rank, file, 0, 1);
	n_moves += moves_in_direction;
	
	moves_in_direction = find_all_possible_moves_in_direction(position, into, rank, file, 0, -1);
	n_moves += moves_in_direction;

	return n_moves;
}

int find_all_possible_queen_moves(struct position *position, struct move **into, int rank, int file) {
	assert(rank >= 0 && rank <= 7);
	assert(file >= 0 && rank <= 7);
	assert(position->squares[rank][file].has_piece);
	assert(position->squares[rank][file].piece_type == PIECE_TYPE_QUEEN);

	int n_moves = 0;

	// towards rank 7
	int moves_in_direction = find_all_possible_moves_in_direction(position, into, rank, file, 1, 0);
	n_moves += moves_in_direction;

	// toward rank 0
	moves_in_direction = find_all_possible_moves_in_direction(position, into, rank, file, -1, 0);
	n_moves += moves_in_direction;
	
	// toward file 7
	moves_in_direction = find_all_possible_moves_in_direction(position, into, rank, file, 0, 1);
	n_moves += moves_in_direction;
	
	// toward file 0
	moves_in_direction = find_all_possible_moves_in_direction(position, into, rank, file, 0, -1);
	n_moves += moves_in_direction;

	// +1, +1 diagonal direction
	moves_in_direction = find_all_possible_moves_in_direction(position, into, rank, file, 1, 1);
	n_moves += moves_in_direction;

	// -1, +1 diagonal direction
	moves_in_direction = find_all_possible_moves_in_direction(position, into, rank, file, -1, 1);
	n_moves += moves_in_direction;

	// -1, -1 diagonal direction
	moves_in_direction = find_all_possible_moves_in_direction(position, into, rank, file, -1, -1);
	n_moves += moves_in_direction;

	// +1, -1 diagonal direction
	moves_in_direction = find_all_possible_moves_in_direction(position, into, rank, file, 1, -1);
	n_moves += moves_in_direction;

	return n_moves;
}

// returns the number of possible moves a king located at [rank][file] on the position can make
// places the possible moves into the *into param, if into is NULL, it just counts the number of moves without recording them
int find_all_possible_king_moves(struct position *position, struct move **into, int king_rank, int king_file) {
	assert(king_rank >= 0 && king_rank <= 7);
	assert(king_file >= 0 && king_file <= 7);
	assert(position->squares[king_rank][king_file].has_piece);
	assert(position->squares[king_rank][king_file].piece_type == PIECE_TYPE_KING);

	bool is_king_white = position->squares[king_rank][king_file].is_piece_white;

	struct move next_move = {0};
	next_move.piece_type = PIECE_TYPE_KING;
	next_move.is_piece_white = is_king_white;
	next_move.source_rank = king_rank;
	next_move.source_file = king_file;

	int n_moves = 0;

	for (int i = 0; i < 8; i++) {
		int target_rank = king_rank + king_move_rank_offsets[i];
		int target_file = king_file + king_move_file_offsets[i];

		if (target_rank < 0 || target_rank > 7 || target_file < 0 || target_file > 7)
			continue;

		struct square target_square = position->squares[target_rank][target_file];

		next_move.target_rank = target_rank;
		next_move.target_file = target_file;

		if (target_square.has_piece) {
			if (target_square.is_piece_white != is_king_white) {
				next_move.is_capture = true;
				next_move.captured_piece_type = target_square.piece_type;

				finalize_move_info_and_record_if_legal(position, &next_move, into, &n_moves);
			}
			continue;
		} else {
			next_move.is_capture = false;
			finalize_move_info_and_record_if_legal(position, &next_move, into, &n_moves);
		}
	}

	// castling moves
	// TODO: factor out the logic, lots of duplication here
	{
		next_move.is_capture = false;

		// in all the below situations, we need to check the following:
		// 1. that we have our castling rights in the appropriate direction
		// 2. that we are not castling through check
		// 3. that there are no pieces in the way between the king's initial position and the rook's initial position
		if (is_king_white) {
			if (king_rank == 0 && king_file == 4) {
				if (position->white_can_castle_kingside) {
					// we can't castle if f1 is under attack, since that would be castling through check
					bool is_f1_hit = is_square_attacked_by_piece_of_color(position, 0, 5, !is_king_white);
					if (!is_f1_hit) {
						// check f1 & g1 for pieces
						if (!position->squares[0][5].has_piece && !position->squares[0][6].has_piece) {
							// we should be able to castle at this point
							next_move.target_rank = 0;
							next_move.target_file = 6;
							finalize_move_info_and_record_if_legal(position, &next_move, into, &n_moves);
						}
					}
				}

				if (position->white_can_castle_queenside) {
					// we can't castle if d1 is under attack, since that would be castling through check
					bool is_d1_hit = is_square_attacked_by_piece_of_color(position, 0, 3, !is_king_white);
					if (!is_d1_hit) {
						// check b1, c1, d1 for pieces
						if (!position->squares[0][1].has_piece && !position->squares[0][2].has_piece && !position->squares[0][3].has_piece) {
							// we should be able to castle at this point
							next_move.target_rank = 0;
							next_move.target_file = 2;
							finalize_move_info_and_record_if_legal(position, &next_move, into, &n_moves);
						}
					}
				}
			}
		} else {
			if (king_rank == 7 && king_file == 4) {
				if (position->black_can_castle_kingside) {
					// we can't castle if f8 is under attack, since that would be castling through check
					bool is_f8_hit = is_square_attacked_by_piece_of_color(position, 7, 5, !is_king_white);
					if (!is_f8_hit) {
						// check f8 & g8 for pieces
						if (!position->squares[7][5].has_piece && !position->squares[7][6].has_piece) {
							// we should be able to castle at this point
							next_move.target_rank = 7;
							next_move.target_file = 6;
							finalize_move_info_and_record_if_legal(position, &next_move, into, &n_moves);
						}
					}
				}

				if (position->black_can_castle_queenside) {
					// we can't castle if d8 is under attack, since that would be castling through check
					bool is_d8_hit = is_square_attacked_by_piece_of_color(position, 7, 3, !is_king_white);
					if (!is_d8_hit) {
						// check b8, c8, d8 for pieces
						if (!position->squares[7][1].has_piece && !position->squares[7][2].has_piece && !position->squares[7][3].has_piece) {
							// we should be able to castle at this point
							next_move.target_rank = 7;
							next_move.target_file = 2;
							finalize_move_info_and_record_if_legal(position, &next_move, into, &n_moves);
						}
					}
				}
			}
		}
	}
	
	return n_moves;
}

int find_all_possible_moves_for_piece(struct position *position, struct move *into, int rank, int file) {
	piece_type piece_type = position->squares[rank][file].piece_type;
	
	int n_piece_moves;
	switch (piece_type) {
		case PIECE_TYPE_PAWN: {
			n_piece_moves = find_all_possible_pawn_moves(position, &into, rank, file);
		}
		break;

		case PIECE_TYPE_KNIGHT: {
			n_piece_moves = find_all_possible_knight_moves(position, &into, rank, file);
		};
		break;

		case PIECE_TYPE_BISHOP: {
			n_piece_moves = find_all_possible_bishop_moves(position, &into, rank, file);
		};
		break;

		case PIECE_TYPE_ROOK: {
			n_piece_moves = find_all_possible_rook_moves(position, &into, rank, file);
		};
		break;

		case PIECE_TYPE_QUEEN: {
			n_piece_moves = find_all_possible_queen_moves(position, &into, rank, file);
		};
		break;

		case PIECE_TYPE_KING: {
			n_piece_moves = find_all_possible_king_moves(position, &into, rank, file);
		};
		break;

		default:
			fprintf(stderr, "find_all_possible_moves_for_piece: got illegal piece %d at %d %d\n", piece_type, rank, file);
			exit(1);
	}
	
	return n_piece_moves;
}

// returns the total count of all possible moves on the position for the provided piece_color
// places the legal moves into the into arg, if one is provided
// if into is NULL, it just returns the count of moves without trying to record them
int find_all_possible_moves_for_color(struct position *position, struct move *into, bool is_color_white) {
	int n_moves = 0;

	for (int rank = 0; rank < 8; rank++) {
		for (int file = 0; file < 8; file++) {
			struct square current_square = position->squares[rank][file];

			if (!current_square.has_piece)
				continue;
			
			if (current_square.is_piece_white != is_color_white)
				continue;
	
			int n_piece_moves = find_all_possible_moves_for_piece(position, into, rank, file);
			n_moves += n_piece_moves;
			if (into != NULL)
				into += n_piece_moves;
		}
	}

	return n_moves;
}

// returns whether the move provided mates the opposing king
int is_move_check_or_mate(struct position *position, struct move *move) {
	apply_move_to_position(position, move);

	bool is_white_move = move->is_piece_white;

	int king_rank, king_file;
	get_king_position(position, !is_white_move, &king_rank, &king_file);

	bool is_check = is_king_on_square_in_check(position, king_rank, king_file);
	
	// do not want to record all possible moves, just want a count, so pass NULL for the 2nd arg
	int n_moves = find_all_possible_moves_for_color(position, NULL, !is_white_move);

	undo_move_from_position(position, move);
	
	if (is_check) {
		if (n_moves == 0)
			return MOVE_IS_MATE;
		return MOVE_IS_CHECK;
	}
	return MOVE_IS_NOT_CHECK_OR_MATE;
}

/*
int main(void) {
	struct position position;
	memset(&position, 0, sizeof(position));
	
	char *starting_position_fen = "rnbqkbnr/pppppppp/8/8/4R3/8/PPPPPPPP/RNBQKBNR w KQkq - 0 1";
	char *position_fen = "r1b2rk1/p1nq2bp/8/3p4/1N2p3/1PN3P1/P2P2BP/R2Q1RK1 b - - 1 18";
	char *pinned_knight = "rnbqk1nr/pppp1ppp/8/4p3/1b1P4/2N5/PPP1PPPP/R1BQKBNR w KQkq - 2 3";
	char *position_with_105_mates_for_white = "1B1Q1Q2/2R5/pQ4QN/RB2k3/1Q5Q/N4Q2/K2Q4/6Q1 w - -";
	char *mates_in_one = "k7/7Q/7Q/8/8/8/8/7K";
	char *most_possible_moves_for_white = "R6R/3Q4/1Q4Q1/4Q3/2Q4Q/Q4Q2/pp1Q4/kBNN1KB1 w - -";
	char *en_passant_to_the_left_possible = "rnbqkbnr/ppp1ppp1/7p/3pP3/8/8/PPPP1PPP/RNBQKBNR w KQkq d6 0 3";
	char *en_passant_to_the_right_possible = "rnbqkbnr/ppppp1p1/7p/4Pp2/8/8/PPPP1PPP/RNBQKBNR w KQkq f6 0 3";
	char *white_can_castle_kingside_and_queenside = "rnbqkbnr/ppp2ppp/3pp3/8/2BPP1Q1/2N1BN2/PPP2PPP/R3K2R b KQ - 6 7";
	char *white_has_32_promotion_moves = "8/PPPPPPPP/8/8/8/7k/K7/8 w - - 0 1";
	
	load_fen_to_position(white_has_32_promotion_moves, &position);

	struct move moves[256];
	printf("%s\n\n", position_str(&position));

	int n_moves = find_all_possible_moves_for_color(&position, moves, PIECE_COLOR_WHITE);
	printf("n moves: %d\n", n_moves);
	for (int i = 0; i < n_moves; i++) {
		printf("%s\n", move_str(&moves[i]));
	}
	
	printf("\n\n");

	int n_mates = 0;
	for (int i = 0; i < n_moves; i++) {
		if (moves[i].is_mate) {
			n_mates++;
			printf("%s\n", move_str(&moves[i]));
		}
	}
	printf("there are %d mates\n", n_mates);
}*/