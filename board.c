#include <assert.h>
#include <stdio.h>
#include <stdint.h>
#include <inttypes.h>
#include <stdlib.h>
#include <string.h>
#include <stdbool.h>



typedef enum { PIECE_TYPE_PAWN, PIECE_TYPE_KNIGHT, PIECE_TYPE_BISHOP, PIECE_TYPE_ROOK, PIECE_TYPE_QUEEN, PIECE_TYPE_KING } piece_type;
typedef enum { PIECE_COLOR_WHITE, PIECE_COLOR_BLACK } piece_color;

piece_color invert_piece_color(piece_color color) {
	if (color == PIECE_COLOR_WHITE)
		return PIECE_COLOR_BLACK;
	return PIECE_COLOR_WHITE;
}

struct square {
	bool has_piece;
	piece_color piece_color;
	piece_type piece_type;
};

#define GAME_ONGOING 0
#define WHITE_WON 1
#define BLACK_WON 2

struct board {
	// [rank][file]
	struct square squares[8][8];

	int white_king_rank;
	int white_king_file;
	int black_king_rank;
	int black_king_file;

	bool is_white_in_check;
	bool is_black_in_check;

	bool white_can_castle_kingside;
	bool black_can_castle_kingside;
	bool white_can_castle_queenside;
	bool black_can_castle_queenside;

	int result;

	bool can_en_passant[8]; // per file, regardless of color

	int n_moves_made; // number of white moves + number of black moves
	struct move *move_history;
};

struct move {
	piece_type piece_type;
	piece_color piece_color;
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
int is_move_check_or_mate(struct board *board, struct move *move);


char *write_move_target_in_algebraic_notation_to_buf(struct move *move, char *to_write_to) {
	if (move->is_capture) {
		*to_write_to = 'x'; to_write_to++;
	}
	// only write the source square if the moved piece is not a king, since there can only be 1 king for each color
	// only write the source square if the moved piece is not a king, since there can only be 1 king for each color
	if (move->piece_type != PIECE_TYPE_KING) {
		*to_write_to = move->source_file + 'a'; to_write_to++;
		*to_write_to = move->source_rank + '1'; to_write_to++;
	}
	*to_write_to = move->target_file + 'a'; to_write_to++;
	*to_write_to = move->target_rank + '1'; to_write_to++;
	return to_write_to;
}


char *move_str(struct move *move) {
	static char move_str_buf[32];
	char *to_write_to = move_str_buf;

	switch (move->piece_type) {
		case PIECE_TYPE_PAWN: {
			
			*to_write_to = move->source_file + 'a'; to_write_to++;
			*to_write_to = move->source_rank + '1'; to_write_to++;
	
			if (move->is_capture) {
				*to_write_to = 'x'; to_write_to++;
			}

			
			*to_write_to = move->target_file + 'a'; to_write_to++;
			*to_write_to = move->target_rank + '1'; to_write_to++;

			if (move->is_promotion) {
				*to_write_to = '='; to_write_to++;
				
				switch (move->piece_type_promoted_to) {
					case PIECE_TYPE_QUEEN:
						*to_write_to = 'Q';
						break;
					case PIECE_TYPE_KNIGHT:
						*to_write_to = 'N';
						break;
					case PIECE_TYPE_BISHOP:
						*to_write_to = 'B';
						break;
					case PIECE_TYPE_ROOK:
						*to_write_to = 'R';
						break;

					default:
						fprintf(stderr, "move->piece_type_promoted_to is %d, which is invalid\n", move->piece_type_promoted_to);
						exit(1);
				}
				to_write_to++;
			}

			if (move->is_en_passant) {
				*to_write_to = 'e'; to_write_to++;
				*to_write_to = 'p'; to_write_to++;
			}
		};
		break;
		
		case PIECE_TYPE_KNIGHT: {
			*to_write_to = 'N'; to_write_to++;
			to_write_to = write_move_target_in_algebraic_notation_to_buf(move, to_write_to);
		};
		break;

		case PIECE_TYPE_ROOK: {
			*to_write_to = 'R'; to_write_to++;
			to_write_to = write_move_target_in_algebraic_notation_to_buf(move, to_write_to);
		};
		break;

		case PIECE_TYPE_BISHOP: {
			*to_write_to = 'B'; to_write_to++;
			to_write_to = write_move_target_in_algebraic_notation_to_buf(move, to_write_to);
		};
		break;

		case PIECE_TYPE_QUEEN: {
			*to_write_to = 'Q'; to_write_to++;
			to_write_to = write_move_target_in_algebraic_notation_to_buf(move, to_write_to);
		};
		break;

		case PIECE_TYPE_KING: {
			if (move->source_file - move->target_file == -2) { // kingside castle, source file is always 4, target is 6
				assert(move->source_file == 4);
				assert(move->target_file == 6);

				strcpy(to_write_to, "O-O");
				to_write_to += 3;
			} else if (move->source_file - move->target_file == 2) { // queenside castle, source file is always 4, target is 2
				assert(move->source_file == 4);
				assert(move->target_file == 2);

				strcpy(to_write_to, "O-O-O");
				to_write_to += 5;
			} else {
				*to_write_to = 'K'; to_write_to++;
				to_write_to = write_move_target_in_algebraic_notation_to_buf(move, to_write_to);
			}
		};
		break;

		default:
			fprintf(stderr, "move_str received unknown piece type %d\n", move->piece_type);
			exit(1);
	}
	
	if (move->is_mate) {
		*to_write_to = '#'; to_write_to++;
	} else if (move->is_check) {
		*to_write_to = '+'; to_write_to++;
	}
	
	*to_write_to = 0;
	
	return move_str_buf;
}



char *board_str(const struct board *board) {
	static char board_str_buf[512];
	char *to_write_to = board_str_buf;

	for (int rank = 7; rank >= 0; rank--) {
		*to_write_to = rank + 1 + '0'; to_write_to++;
		
		*to_write_to = ' '; to_write_to++;
		*to_write_to = ' '; to_write_to++;

		for (int file = 0; file < 8; file++) {
			struct square square = board->squares[rank][file];
			
			if (!square.has_piece) {
				*to_write_to = ' '; to_write_to++;
				continue;
			}

			
			piece_type piece_type = square.piece_type;
			
			char piece_ch;
			switch (square.piece_type) {
			case PIECE_TYPE_PAWN: piece_ch = 'P'; break;
			case PIECE_TYPE_KNIGHT: piece_ch = 'N'; break;
			case PIECE_TYPE_BISHOP: piece_ch = 'B'; break;
			case PIECE_TYPE_ROOK: piece_ch = 'R'; break;
			case PIECE_TYPE_QUEEN: piece_ch = 'Q'; break;
			case PIECE_TYPE_KING: piece_ch = 'K'; break;

			default:
				fprintf(stderr, "board_str: unknown piece type %"PRIx8" at [%d, %d]\n", piece_type, rank, file);
				exit(1);
			}
			
			piece_color piece_color = square.piece_color;
			if (piece_color == PIECE_COLOR_BLACK)
				piece_ch += 32;
			
			*to_write_to = piece_ch; to_write_to++;
		} // file loop
		*to_write_to = '\n'; to_write_to++;
	} // rank loop
	
	*to_write_to = ' '; to_write_to++;
	*to_write_to = ' '; to_write_to++;
	*to_write_to = ' '; to_write_to++;
	for (char file_ch = 'a'; file_ch <= 'h'; file_ch++) {
		*to_write_to = file_ch; to_write_to++;
	}

	*to_write_to = 0;

	return board_str_buf;
}

void load_fen_to_board(const char *fen, struct board *into) {

	into->white_king_rank = -1;
	into->white_king_file = -1;
	into->black_king_rank = -1;
	into->black_king_file = -1;

	for (int rank = 7; rank >= 0; rank--) {
		for (int file = 0; file < 8; ) {
			char ch = *fen;

			if (ch >= '1' && ch <= '8') {
				int n_empty_squares = ch - '0';
				file += n_empty_squares;
			} else {
					
				struct square *current_square = &into->squares[rank][file];

				current_square->has_piece = true;
				
				if (ch == 'p' || ch == 'n' || ch == 'b' || ch == 'r' || ch == 'q' || ch == 'k') {
					current_square->piece_color = PIECE_COLOR_BLACK;
					ch -= 32;
				} else {
					current_square->piece_color = PIECE_COLOR_WHITE;
				}
				
				switch (ch) {
				case 'P':
				current_square->piece_type = PIECE_TYPE_PAWN; break;
				case 'N': 
				current_square->piece_type = PIECE_TYPE_KNIGHT; break;
				case 'B': 
				current_square->piece_type = PIECE_TYPE_BISHOP; break;
				case 'R': 
				current_square->piece_type = PIECE_TYPE_ROOK; break;
				case 'Q': 
				current_square->piece_type = PIECE_TYPE_QUEEN; break;
				case 'K': {
					current_square->piece_type = PIECE_TYPE_KING; 
					if (current_square->piece_color == PIECE_COLOR_WHITE) {
						into->white_king_rank = rank;
						into->white_king_file = file;
					} else {
						into->black_king_rank = rank;
						into->black_king_file = file;
					}
				} 
				break;
				default:
					fprintf(stderr, "found invalid character '%c' in fen\n", ch);
					exit(1);
				}
	
				file++;
				
			}
			fen++;
		} // file loop
		
		if (rank > 0) {
			char ch = *fen;
			if (ch != '/') {
				fprintf(stderr, "malformed fen, expected '/' character but found '%c' after reading rank %d\n", ch, rank+1);
				exit(1);
			}
		}
		fen++;

	} // rank loop

	assert(into->white_king_rank != -1);
	assert(into->white_king_file != -1);
	assert(into->black_king_rank != -1);
	assert(into->black_king_file != -1);

	// whose turn is it portion of the fen
	fen += 2;

	into->white_can_castle_kingside = false;
	into->black_can_castle_kingside = false;
	into->white_can_castle_queenside = false;
	into->black_can_castle_queenside = false;
	// read through the castling rights portion of the fen
	while (*fen != ' ') {
		switch (*fen) {
			case 'K': into->white_can_castle_kingside = true; break;
			case 'k': into->black_can_castle_kingside = true; break;
			case 'Q': into->white_can_castle_queenside = true; break;
			case 'q': into->black_can_castle_queenside = true; break;
			case '-': break;
			default:
				fprintf(stderr, "fen contains invalid character '%c' in castling rights portion\n", *fen);
				exit(1);
		}

		fen++;
	}
	fen++; // white space after castling portion of fen

	memset(into->can_en_passant, 0, 8 * sizeof(into->can_en_passant[0]));
	if (*fen != '-') {
		char file_char = *fen;
		int file_to_en_passant = file_char - 'a';
		into->can_en_passant[file_to_en_passant] = true;
	}
}

int int_difference(int a, int b) {
	int signed_diff = a - b;
	if (signed_diff < 0)
		return -signed_diff;
	return signed_diff;
}


void get_king_position(const struct board *board, piece_color king_color, int *rank, int *file) {
	if (king_color == PIECE_COLOR_WHITE) {
		*rank = board->white_king_rank;
		*file = board->white_king_file;
	} else {
		*rank = board->black_king_rank;
		*file = board->black_king_file;
	}
}

static struct board saved_board_states[256];
static int n_saved_board_states = 0;


void modify_squares_for_castled_rook(struct square *source_sq, struct square *target_sq, piece_color rook_color) {
	source_sq->has_piece = false;

	target_sq->has_piece = true;
	target_sq->piece_type = PIECE_TYPE_ROOK;
	target_sq->piece_color = rook_color;
}

// applies move to board without any constraints, the move should be legal if the state of the board intends to be correct after
void apply_move_to_board(struct board *board, const struct move *move) {
	saved_board_states[n_saved_board_states] = *board;
	n_saved_board_states++;

	// check for whether the move is a castle, since the rook castled with needs to move here
	// this only moves the rook that is being castled with, the king's move is taken care of by the general piece move code below
	if (move->piece_type == PIECE_TYPE_KING) {
		if (move->piece_color == PIECE_COLOR_WHITE) {
			board->white_king_rank = move->target_rank;
			board->white_king_file = move->target_file;

			// if the source square is the king's starting square && target square is a castled position
			// AND the king can castle in that direction
			// then we move both the king and rook to the appropriate squares
			// e.g. king move from e1 to g1 means we move rook from h1 to f1 as well
			if (move->source_rank == 0 && move->source_file == 4) {
				
				if (move->target_rank == 0 && move->target_file == 6) { // white kingside castles
					struct square *h1_square = &board->squares[0][7];
					struct square *f1_square = &board->squares[0][5];

					modify_squares_for_castled_rook(h1_square, f1_square, move->piece_color);

				} else if (move->target_rank == 0 && move->target_file == 2) { // white queenside castles
					struct square *a1_square = &board->squares[0][0];
					struct square *d1_square = &board->squares[0][3];

					modify_squares_for_castled_rook(a1_square, d1_square, move->piece_color);
				}
			}

			// the moving side's king loses castling rights, regardless of the type of king move made here
			board->white_can_castle_kingside = false;
			board->white_can_castle_queenside = false;
		} else {
			board->black_king_rank = move->target_rank;
			board->black_king_file = move->target_file;

			// if king is moving from it's starting square of e8
			if (move->source_rank == 7 && move->source_file == 4) {
				if (move->target_rank == 7 && move->target_file == 6) { // white kingside castles
					struct square *h8_square = &board->squares[7][7];
					struct square *f8_square = &board->squares[7][5];

					modify_squares_for_castled_rook(h8_square, f8_square, move->piece_color);

				} else if (move->target_rank == 7 && move->target_file == 2) { // white queenside castles
					struct square *a8_square = &board->squares[7][0];
					struct square *d8_square = &board->squares[7][3];

					modify_squares_for_castled_rook(a8_square, d8_square, move->piece_color);
				}
			}

			// the moving side's king loses castling rights, regardless of the type of king move made here
			board->black_can_castle_kingside = false;
			board->black_can_castle_queenside = false;
		}

	} else if (move->piece_type == PIECE_TYPE_ROOK) {

		// a rook move that originates from the rook's original square (i.e. a1, h1, a8, h8)
		// causes the appropriate castling rights to be revoked
		if (move->source_rank == 0) {
			if (move->source_file == 0)
				board->white_can_castle_queenside = false;
			else if (move->source_file == 7)
				board->white_can_castle_kingside = false;

		} else if (move->source_rank == 7) {
			if (move->source_file == 0)
				board->black_can_castle_queenside = false;
			else if (move->source_file == 7)
				board->black_can_castle_kingside = false;
		}
	}

	assert(move->source_rank >= 0);
	assert(move->source_rank <= 7);
	assert(move->source_file >= 0);
	assert(move->source_file <= 7);
	assert(move->target_rank >= 0);
	assert(move->target_rank <= 7);
	assert(move->target_file >= 0);
	assert(move->target_file <= 7);

	struct square *source_square = &board->squares[move->source_rank][move->source_file];
	struct square *target_square = &board->squares[move->target_rank][move->target_file];

	source_square->has_piece = false;
	
	target_square->has_piece = true;
	target_square->piece_color = move->piece_color;

	if (move->is_promotion) {
		target_square->piece_type = move->piece_type_promoted_to;
	} else {
		target_square->piece_type = move->piece_type;
	}

	// all previous en passant possibilities are gone after a move is made, there can only be one possibility on the next move
	// that's only if the current move is a pawn move 2 squares forward
	memset(board->can_en_passant, 0, 8 * sizeof(board->can_en_passant[0])); 
	if (move->piece_type == PIECE_TYPE_PAWN) {
		if (int_difference(move->source_rank, move->target_rank) == 2) {
			board->can_en_passant[move->target_file] = true;
		}
	}

	if (move->is_mate) {
		if (move->piece_color == PIECE_COLOR_WHITE) {
			board->result = WHITE_WON;
		} else {
			board->result = BLACK_WON;
		}
	} else if (move->is_check) {
		if (move->piece_color == PIECE_COLOR_WHITE) {
			board->is_black_in_check = true;
			board->is_white_in_check = false; // it is not possible for the move to have been made by white and for white to be in check after the move
		} else {
			board->is_white_in_check = true;
			board->is_black_in_check = false; // it is not possible for the move to have been made by black and for black to be in check after the move
		}
	}
}

void undo_move_from_board(struct board *board, const struct move *move) {
	n_saved_board_states--;

	if (n_saved_board_states < 0) {
		fprintf(stderr, "n_saved_states is < 0 during undo_move_from_board\n");
		exit(1);
	}
	*board = saved_board_states[n_saved_board_states];
}

// returns whether the square [rank][file] is directly attacked by a piece of a provided color
// from the provided direction
// the rank_dir and file_dir should both be in [-1, 1], otherwise the result doesn't really make sense
bool is_square_attacked_from_direction_by_color(const struct board *board, piece_color color, int rank, int file, int rank_dir, int file_dir) {
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

	// loop while we are within bounds of the board, if we exit bounds, then the square is not under attack from the direction
	while (target_rank >= 0 && target_rank <= 7 && target_file >= 0 && target_file <= 7) {		
		struct square target_square = board->squares[target_rank][target_file];

		if (target_square.has_piece) {
			if (target_square.piece_color != color) {
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
					if (color == PIECE_COLOR_WHITE) {
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
bool is_square_attacked_by_piece_of_color(const struct board *board, int rank, int file, piece_color color) {
	assert(rank >= 0);
	assert(rank <= 7);
	assert(file >= 0);
	assert(rank <= 7);

	// diagonal checks for bishop or queen attack, also includes attacks by pawn & king, which can only apply after 1 step in the direction
	{
		// -1, -1 direction
		if (is_square_attacked_from_direction_by_color(board, color, rank, file, -1, -1))
			return true;

		// -1, 1
		if (is_square_attacked_from_direction_by_color(board, color, rank, file, -1, 1))
			return true;

		// 1, -1 direction
		if (is_square_attacked_from_direction_by_color(board, color, rank, file, 1, -1))
			return true;

		// 1, 1 direction
		if (is_square_attacked_from_direction_by_color(board, color, rank, file, 1, 1))
			return true;
	}

	// attacks along a rank or file by either a queen or rook, also includes attacks by a king, which can only apply after 1 step in the direction
	{
		// 1, 0 direction
		if (is_square_attacked_from_direction_by_color(board, color, rank, file, 1, 0))
			return true;

		// -1, 0 direction
		if (is_square_attacked_from_direction_by_color(board, color, rank, file, -1, 0))
			return true;

		// 0, 1 direction
		if (is_square_attacked_from_direction_by_color(board, color, rank, file, 0, 1))
			return true;

		// 0, -1 direction
		if (is_square_attacked_from_direction_by_color(board, color, rank, file, 0, -1))
			return true;
	}

	// attacks by a knight
	{
		for (int i = 0; i < 8; i++) {
			int knight_rank = rank + knight_move_rank_offsets[i];
			int knight_file = file + knight_move_file_offsets[i];
	
			if (knight_rank < 0 || knight_rank > 7 || knight_file < 0 || knight_file > 7)
				continue;

			struct square the_square = board->squares[knight_rank][knight_file];
			
			if (the_square.has_piece && the_square.piece_color == color && the_square.piece_type == PIECE_TYPE_KNIGHT)
				return true;
		}
	}

	return false;
}

// returns whether the king located at king_rank, king_file is in check on the provided board
bool is_king_on_square_in_check(const struct board *board, int king_rank, int king_file) {
	assert(board->squares[king_rank][king_file].has_piece);
	assert(board->squares[king_rank][king_file].piece_type == PIECE_TYPE_KING);
	
	piece_color king_color = board->squares[king_rank][king_file].piece_color;
	piece_color opposite_color = invert_piece_color(king_color);

	// just check if the king's square is attacked by a piece of the opposite color
	return is_square_attacked_by_piece_of_color(board, king_rank, king_file, opposite_color);
}

bool is_move_legal(struct board *board, struct move *move) {
	apply_move_to_board(board, move);

	int king_rank, king_file;
	get_king_position(board, move->piece_color, &king_rank, &king_file);

	// a move is illegal if it puts the mover's side's king in check
	bool is_illegal = is_king_on_square_in_check(board, king_rank, king_file);
	
	undo_move_from_board(board, move);

	return !is_illegal;
}

// does the following steps:
// checks if the move is legal
// if it is, checks if it the move is a check or mate
// if *intop is not NULL, records the move there, increments *intop, and increments *n_moves
void finalize_move_info_and_record_if_legal(struct board *board, struct move *move, struct move **intop, int *n_moves) {
	struct move *into = *intop;
	move->is_check = false;
	move->is_mate = false;
	if (is_move_legal(board, move)) {
		if (into != NULL) {
			int move_result = is_move_check_or_mate(board, move);
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


int find_all_possible_pawn_moves(struct board *board, struct move **into, int rank, int file) {
	assert(rank >= 0 && rank <= 7);
	assert(file >= 0 && file <= 7);
	assert(board->squares[rank][file].has_piece);
	assert(board->squares[rank][file].piece_type == PIECE_TYPE_PAWN);

	piece_color pawn_color = board->squares[rank][file].piece_color;

	// next_rank for a pawn move of 1 square forward
	int next_rank;
	if (pawn_color == PIECE_COLOR_WHITE) {
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
	next_move.piece_color = pawn_color;

	piece_type possible_promotions[4] = { PIECE_TYPE_QUEEN, PIECE_TYPE_ROOK, PIECE_TYPE_BISHOP, PIECE_TYPE_KNIGHT };

	// move forward one square logic
	{
		struct square square_in_front_of_pawn = board->squares[next_rank][file];
		if (!square_in_front_of_pawn.has_piece) {
			next_move.target_rank = next_rank;
			next_move.target_file = file;
			next_move.is_capture = false;
			next_move.is_en_passant = false;

			// promotion case for pawn
			if (next_rank == 0 || next_rank == 7) {
				next_move.is_promotion = true;

				for (int i = 0; i < 4; i++) {
					next_move.piece_type_promoted_to = possible_promotions[i];

					finalize_move_info_and_record_if_legal(board, &next_move, into, &n_moves);
				}
			} else { // forward move without promotion
				next_move.is_promotion = false;
				finalize_move_info_and_record_if_legal(board, &next_move, into, &n_moves);
			}

		}
	}

	// capture diagonally to the left
	{
		int left_file;
		if (pawn_color == PIECE_COLOR_WHITE)
			left_file = file - 1;
		else
			left_file = file + 1;
		
		
		if (left_file >= 0 && left_file <= 7) {
			struct square target_square = board->squares[next_rank][left_file];
		
			if (target_square.has_piece && target_square.piece_color != pawn_color) {
				// we should never end up in situation where the target square has a king and the pawn can capture it
				assert(target_square.piece_type != PIECE_TYPE_KING);

				next_move.is_capture = true;
				next_move.target_rank = next_rank;
				next_move.target_file = left_file;
				next_move.captured_piece_type = target_square.piece_type;
				next_move.is_en_passant = false;

				if (next_rank == 0 || next_rank == 7) {
					next_move.is_promotion = true;

					for (int i = 0; i < 4; i++) {
						next_move.piece_type_promoted_to = possible_promotions[i];
						finalize_move_info_and_record_if_legal(board, &next_move, into, &n_moves);
					}

				} else {
					next_move.is_promotion = false;
					finalize_move_info_and_record_if_legal(board, &next_move, into, &n_moves);
				}	

				finalize_move_info_and_record_if_legal(board, &next_move, into, &n_moves);
			}
		}
	}

	// capture diagonally to the right
	{
		int right_file;
		if (pawn_color == PIECE_COLOR_WHITE)
			right_file = file + 1;
		else
			right_file = file - 1;

		if (right_file >= 0 && right_file <= 7) {
			struct square target_square = board->squares[next_rank][right_file];
		
			if (target_square.has_piece && target_square.piece_color != pawn_color) {
				// we should never end up in situation where the target square has a king of the opposite color and the pawn can capture it
				assert(target_square.piece_type != PIECE_TYPE_KING);

				next_move.is_capture = true;
				next_move.target_rank = next_rank;
				next_move.target_file = right_file;
				next_move.captured_piece_type = target_square.piece_type;
				next_move.is_en_passant = false;

				if (next_rank == 0 || next_rank == 7) {
					next_move.is_promotion = true;

					for (int i = 0; i < 4; i++) {
						next_move.piece_type_promoted_to = possible_promotions[i];
						finalize_move_info_and_record_if_legal(board, &next_move, into, &n_moves);
					}

				} else {
					next_move.is_promotion = false;
					finalize_move_info_and_record_if_legal(board, &next_move, into, &n_moves);
				}	
			}
		}
	}

	// forward 2 squares logic
	{
		int target_rank = -1;
		if (pawn_color == PIECE_COLOR_WHITE && rank == 1) {
			target_rank = rank + 2;
		} else if (pawn_color == PIECE_COLOR_BLACK && rank == 6) {
			target_rank = rank - 2;
		}

		if (target_rank != -1 && !board->squares[target_rank][file].has_piece && !board->squares[target_rank-1][file].has_piece) {
			next_move.is_capture = false;
			next_move.is_promotion = false;
			next_move.target_rank = target_rank;
			next_move.target_file = file;
			next_move.is_en_passant = false;

			finalize_move_info_and_record_if_legal(board, &next_move, into, &n_moves);
		}
		
	}

	// en passant to the left of the pawn
	{
		int left_file;
		if (pawn_color == PIECE_COLOR_WHITE)
			left_file = file - 1;
		else
			left_file = file + 1;
		
		if (left_file >= 0 && left_file <= 7) {
			
			struct square target_square = board->squares[rank][left_file];

			bool can_en_passant = target_square.has_piece && target_square.piece_color != pawn_color && target_square.piece_type == PIECE_TYPE_PAWN && board->can_en_passant[left_file];
			if (can_en_passant) {
				next_move.is_capture = true;
				next_move.captured_piece_type = PIECE_TYPE_PAWN;
				next_move.target_rank = next_rank;
				next_move.target_file = left_file;
				next_move.is_promotion = false;
				next_move.is_en_passant = true;

				finalize_move_info_and_record_if_legal(board, &next_move, into, &n_moves);
			}
		}
	}

	// en passant to the right of the pawn
	{
		int right_file;
		if (pawn_color == PIECE_COLOR_WHITE)
			right_file = file + 1;
		else
			right_file = file - 1;
		
		if (right_file >= 0 && right_file <= 7) {
			
			struct square target_square = board->squares[rank][right_file];

			bool can_en_passant = target_square.has_piece && target_square.piece_color != pawn_color && target_square.piece_type == PIECE_TYPE_PAWN && board->can_en_passant[right_file];
			if (can_en_passant) {
				next_move.is_capture = true;
				next_move.captured_piece_type = PIECE_TYPE_PAWN;
				next_move.target_rank = next_rank;
				next_move.target_file = right_file;
				next_move.is_promotion = false;
				next_move.is_en_passant = true;

				finalize_move_info_and_record_if_legal(board, &next_move, into, &n_moves);
			}
		}
	}

	return n_moves;
}



int find_all_possible_knight_moves(struct board *board, struct move **into, int rank, int file) {
	assert(rank >= 0 && rank <= 7);
	assert(file >= 0 && rank <= 7);
	assert(board->squares[rank][file].has_piece);
	assert(board->squares[rank][file].piece_type == PIECE_TYPE_KNIGHT);

	int n_moves = 0;

	piece_color knight_color = board->squares[rank][file].piece_color;

	struct move next_move = {0};
	next_move.piece_type = PIECE_TYPE_KNIGHT;
	next_move.piece_color = knight_color;
	next_move.source_rank = rank;
	next_move.source_file = file;
	
	for (int i = 0; i < 8; i++) {
		int target_rank = rank + knight_move_rank_offsets[i];
		int target_file = file + knight_move_file_offsets[i];
	
		if (target_rank >= 0 && target_rank <= 7 && target_file >= 0 && target_file <= 7) {
		
			struct square target_square = board->squares[target_rank][target_file];
			
			next_move.target_rank = target_rank;
			next_move.target_file = target_file;
			
			if (target_square.has_piece) {
				if (target_square.piece_color == knight_color) {
					continue;
				}
		
				// we should never end up in situation where the target square has a king and the knight can capture it
				assert(target_square.piece_type != PIECE_TYPE_KING);
				
				next_move.is_capture = true;
				next_move.captured_piece_type = target_square.piece_type;
			} else {
				next_move.is_capture = false;
			}

			finalize_move_info_and_record_if_legal(board, &next_move, into, &n_moves);
		}
	}

	return n_moves;
}

int find_all_possible_moves_in_direction(struct board *board, struct move **into, int rank, int file, int rank_dir, int file_dir) {
	assert(rank >= 0 && rank <= 7);
	assert(file >= 0 && rank <= 7);
	assert(board->squares[rank][file].has_piece);

	piece_type moved_piece_type = board->squares[rank][file].piece_type;
	piece_color moved_piece_color = board->squares[rank][file].piece_color;

	struct move next_move = {0};
	next_move.piece_type = moved_piece_type;
	next_move.piece_color = moved_piece_color;
	next_move.source_rank = rank;
	next_move.source_file = file;
	
	int target_rank = rank + rank_dir;
	int target_file = file + file_dir;

	int n_moves = 0;

	while (1) {
		if (target_rank < 0 || target_rank > 7 || target_file < 0 || target_file > 7) // we went out of bounds
			break;
	
		struct square target_square = board->squares[target_rank][target_file];
			
		next_move.target_rank = target_rank;
		next_move.target_file = target_file;

		if (target_square.has_piece) {
			if (target_square.piece_color != moved_piece_color) {
				// we should never end up in situation where the target square has a king, this is an invalid state
				if (target_square.piece_type == PIECE_TYPE_KING) {
					fprintf(stderr, "find_all_possible_moves_in_direction target square has a king of the opposite color, target square: [%d, %d], piece square: [%d, %d], piece: %d\n", 
						target_rank, target_file, rank, file, moved_piece_type);
					exit(1);
				}

				next_move.is_capture = true;
				next_move.captured_piece_type = target_square.piece_type;
				finalize_move_info_and_record_if_legal(board, &next_move, into, &n_moves);
			}
			break;
		} else {
			next_move.is_capture = false;
			finalize_move_info_and_record_if_legal(board, &next_move, into, &n_moves);
		}

		target_rank += rank_dir;
		target_file += file_dir;
	}

	return n_moves;
}

int find_all_possible_bishop_moves(struct board *board, struct move **into, int rank, int file) {
	assert(rank >= 0 && rank <= 7);
	assert(file >= 0 && rank <= 7);
	assert(board->squares[rank][file].has_piece);
	assert(board->squares[rank][file].piece_type == PIECE_TYPE_BISHOP);

	int n_moves = 0;

	// +1, +1 diagonal direction
	int moves_in_direction = find_all_possible_moves_in_direction(board, into, rank, file, 1, 1);
	n_moves += moves_in_direction;

	// -1, +1 diagonal direction
	moves_in_direction = find_all_possible_moves_in_direction(board, into, rank, file, -1, 1);
	n_moves += moves_in_direction;

	// -1, -1 diagonal direction
	moves_in_direction = find_all_possible_moves_in_direction(board, into, rank, file, -1, -1);
	n_moves += moves_in_direction;

	// +1, -1 diagonal direction
	moves_in_direction = find_all_possible_moves_in_direction(board, into, rank, file, 1, -1);
	n_moves += moves_in_direction;

	return n_moves;
}


int find_all_possible_rook_moves(struct board *board, struct move **into, int rank, int file) {
	assert(rank >= 0 && rank <= 7);
	assert(file >= 0 && rank <= 7);
	assert(board->squares[rank][file].has_piece);
	assert(board->squares[rank][file].piece_type == PIECE_TYPE_ROOK);

	int n_moves = 0;

	int moves_in_direction = find_all_possible_moves_in_direction(board, into, rank, file, 1, 0);
	n_moves += moves_in_direction;

	moves_in_direction = find_all_possible_moves_in_direction(board, into, rank, file, -1, 0);
	n_moves += moves_in_direction;
	
	moves_in_direction = find_all_possible_moves_in_direction(board, into, rank, file, 0, 1);
	n_moves += moves_in_direction;
	
	moves_in_direction = find_all_possible_moves_in_direction(board, into, rank, file, 0, -1);
	n_moves += moves_in_direction;

	return n_moves;
}

int find_all_possible_queen_moves(struct board *board, struct move **into, int rank, int file) {
	assert(rank >= 0 && rank <= 7);
	assert(file >= 0 && rank <= 7);
	assert(board->squares[rank][file].has_piece);
	assert(board->squares[rank][file].piece_type == PIECE_TYPE_QUEEN);

	int n_moves = 0;

	// towards rank 7
	int moves_in_direction = find_all_possible_moves_in_direction(board, into, rank, file, 1, 0);
	n_moves += moves_in_direction;

	// toward rank 0
	moves_in_direction = find_all_possible_moves_in_direction(board, into, rank, file, -1, 0);
	n_moves += moves_in_direction;
	
	// toward file 7
	moves_in_direction = find_all_possible_moves_in_direction(board, into, rank, file, 0, 1);
	n_moves += moves_in_direction;
	
	// toward file 0
	moves_in_direction = find_all_possible_moves_in_direction(board, into, rank, file, 0, -1);
	n_moves += moves_in_direction;

	// +1, +1 diagonal direction
	moves_in_direction = find_all_possible_moves_in_direction(board, into, rank, file, 1, 1);
	n_moves += moves_in_direction;

	// -1, +1 diagonal direction
	moves_in_direction = find_all_possible_moves_in_direction(board, into, rank, file, -1, 1);
	n_moves += moves_in_direction;

	// -1, -1 diagonal direction
	moves_in_direction = find_all_possible_moves_in_direction(board, into, rank, file, -1, -1);
	n_moves += moves_in_direction;

	// +1, -1 diagonal direction
	moves_in_direction = find_all_possible_moves_in_direction(board, into, rank, file, 1, -1);
	n_moves += moves_in_direction;

	return n_moves;
}

// returns the number of possible moves a king located at [rank][file] on the board can make
// places the possible moves into the *into param, if into is NULL, it just counts the number of moves without recording them
int find_all_possible_king_moves(struct board *board, struct move **into, int king_rank, int king_file) {
	assert(king_rank >= 0 && king_rank <= 7);
	assert(king_file >= 0 && king_file <= 7);
	assert(board->squares[king_rank][king_file].has_piece);
	assert(board->squares[king_rank][king_file].piece_type == PIECE_TYPE_KING);

	piece_color king_color = board->squares[king_rank][king_file].piece_color;

	struct move next_move = {0};
	next_move.piece_type = PIECE_TYPE_KING;
	next_move.piece_color = king_color;
	next_move.source_rank = king_rank;
	next_move.source_file = king_file;

	int n_moves = 0;

	for (int i = 0; i < 8; i++) {
		int target_rank = king_rank + king_move_rank_offsets[i];
		int target_file = king_file + king_move_file_offsets[i];

		if (target_rank < 0 || target_rank > 7 || target_file < 0 || target_file > 7)
			continue;

		struct square target_square = board->squares[target_rank][target_file];

		next_move.target_rank = target_rank;
		next_move.target_file = target_file;

		if (target_square.has_piece) {
			if (target_square.piece_color != king_color) {
				next_move.is_capture = true;
				next_move.captured_piece_type = target_square.piece_type;

				finalize_move_info_and_record_if_legal(board, &next_move, into, &n_moves);
			}
			continue;
		} else {
			next_move.is_capture = false;
			finalize_move_info_and_record_if_legal(board, &next_move, into, &n_moves);
		}
	}

	// castling moves
	// TODO: factor out the logic, lots of duplication here
	{
		piece_color opposite_color = invert_piece_color(king_color);

		next_move.is_capture = false;

		// in all the below situations, we need to check the following:
		// 1. that we have our castling rights in the appropriate direction
		// 2. that we are not castling through check
		// 3. that there are no pieces in the way between the king's initial position and the rook's initial position
		if (king_color == PIECE_COLOR_WHITE) {
			if (king_rank == 0 && king_file == 4) {
				if (board->white_can_castle_kingside) {
					// we can't castle if f1 is under attack, since that would be castling through check
					bool is_f1_hit = is_square_attacked_by_piece_of_color(board, 0, 5, opposite_color);
					if (!is_f1_hit) {
						// check f1 & g1 for pieces
						if (!board->squares[0][5].has_piece && !board->squares[0][6].has_piece) {
							// we should be able to castle at this point
							next_move.target_rank = 0;
							next_move.target_file = 6;
							finalize_move_info_and_record_if_legal(board, &next_move, into, &n_moves);
						}
					}
				}

				if (board->white_can_castle_queenside) {
					// we can't castle if d1 is under attack, since that would be castling through check
					bool is_d1_hit = is_square_attacked_by_piece_of_color(board, 0, 3, opposite_color);
					if (!is_d1_hit) {
						// check b1, c1, d1 for pieces
						if (!board->squares[0][1].has_piece && !board->squares[0][2].has_piece && !board->squares[0][3].has_piece) {
							// we should be able to castle at this point
							next_move.target_rank = 0;
							next_move.target_file = 2;
							finalize_move_info_and_record_if_legal(board, &next_move, into, &n_moves);
						}
					}
				}
			}
		} else {
			if (king_rank == 7 && king_file == 4) {
				if (board->black_can_castle_kingside) {
					// we can't castle if f8 is under attack, since that would be castling through check
					bool is_f1_hit = is_square_attacked_by_piece_of_color(board, 7, 5, opposite_color);
					if (!is_f1_hit) {
						// check f8 & g8 for pieces
						if (!board->squares[7][5].has_piece && !board->squares[7][6].has_piece) {
							// we should be able to castle at this point
							next_move.target_rank = 7;
							next_move.target_file = 6;
							finalize_move_info_and_record_if_legal(board, &next_move, into, &n_moves);
						}
					}
				}

				if (board->black_can_castle_queenside) {
					// we can't castle if d8 is under attack, since that would be castling through check
					bool is_d1_hit = is_square_attacked_by_piece_of_color(board, 7, 3, opposite_color);
					if (!is_d1_hit) {
						// check b8, c8, d8 for pieces
						if (!board->squares[7][1].has_piece && !board->squares[7][2].has_piece && !board->squares[7][3].has_piece) {
							// we should be able to castle at this point
							next_move.target_rank = 7;
							next_move.target_file = 2;
							finalize_move_info_and_record_if_legal(board, &next_move, into, &n_moves);
						}
					}
				}
			}
		}
	}
	
	return n_moves;
}


// returns the total count of all possible moves on the board for the provided piece_color
// places the legal moves into the into arg, if one is provided
// if into is NULL, it just returns the count of moves without trying to record them
int find_all_possible_moves_for_color(struct board *board, struct move *into, piece_color color) {
	int n_moves = 0;

	for (int rank = 0; rank < 8; rank++) {
		for (int file = 0; file < 8; file++) {
			struct square current_square = board->squares[rank][file];

			if (!current_square.has_piece)
				continue;
	
			piece_color piece_color = current_square.piece_color;
			if (piece_color != color)
				continue;

			piece_type piece_type = current_square.piece_type;
		
			int n_piece_moves;
			switch (piece_type) {
				case PIECE_TYPE_PAWN: {
					n_piece_moves = find_all_possible_pawn_moves(board, &into, rank, file);
				}
				break;

				case PIECE_TYPE_KNIGHT: {
					n_piece_moves = find_all_possible_knight_moves(board, &into, rank, file);
				};
				break;

				case PIECE_TYPE_BISHOP: {
					n_piece_moves = find_all_possible_bishop_moves(board, &into, rank, file);
				};
				break;

				case PIECE_TYPE_ROOK: {
					n_piece_moves = find_all_possible_rook_moves(board, &into, rank, file);
				};
				break;

				case PIECE_TYPE_QUEEN: {
					n_piece_moves = find_all_possible_queen_moves(board, &into, rank, file);
				};
				break;

				case PIECE_TYPE_KING: {
					n_piece_moves = find_all_possible_king_moves(board, &into, rank, file);
				};
				break;

				default:
					assert(false);
			}
			n_moves += n_piece_moves;
		}
	}

	return n_moves;
}

// returns whether the move provided mates the opposing king
int is_move_check_or_mate(struct board *board, struct move *move) {
	apply_move_to_board(board, move);

	piece_color opposite_color = invert_piece_color(move->piece_color);

	int king_rank, king_file;
	get_king_position(board, opposite_color, &king_rank, &king_file);

	bool is_check = is_king_on_square_in_check(board, king_rank, king_file);

	// do not want to record all possible moves, just want a count, so pass NULL for the 2nd arg
	int n_moves = find_all_possible_moves_for_color(board, NULL, opposite_color);

	undo_move_from_board(board, move);
	
	if (is_check) {
		if (n_moves == 0)
			return MOVE_IS_MATE;
		return MOVE_IS_CHECK;
	}
	return MOVE_IS_NOT_CHECK_OR_MATE;
}

int main(void) {
	struct board board;
	memset(&board, 0, sizeof(board));
	
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
	
	load_fen_to_board(white_has_32_promotion_moves, &board);

	struct move moves[256];
	printf("%s\n\n", board_str(&board));

	int n_moves = find_all_possible_moves_for_color(&board, moves, PIECE_COLOR_WHITE);
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
}