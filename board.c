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

struct board {
	// [rank][file]
	struct square squares[8][8];

	bool is_white_in_check;
	bool is_black_in_check;
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
	bool is_promotion; 				   // only applies if piece_type == PIECE_TYPE_PAWN
	piece_type piece_type_promoted_to; // only applies if piece_type == PIECE_TYPE_PAWN
};

// there up to 8 knight moves for a single knight
// each pair of values here are a potential knight move
static int knight_move_rank_offsets[8] = { 2, 1, -1, -2, -2, -1, 1, 2 };
static int knight_move_file_offsets[8] = { 1, 2, 2, 1, -1, -2, -2, -1 };


// a king can move in 8 directions, rank_offsets[i] and file_offsets[i] are one directional pair
static int king_move_rank_offsets[8] = { -1, -1, -1, 0, 1, 1, 1, 0 };
static int king_move_file_offsets[8] = { 1, 0, -1, -1, -1, 0, 1, 1 };

char *write_move_target_in_algebraic_notation_to_buf(struct move *move, char *to_write_to) {
	if (move->is_capture) {
		*to_write_to = 'x'; to_write_to++;
	}
	*to_write_to = move->target_file + 'a'; to_write_to++;
	*to_write_to = move->target_rank + '1'; to_write_to++;
	return to_write_to;
}

static char move_str_buf[32];
char *move_str(struct move *move) {
	char *to_write_to = move_str_buf;

	switch (move->piece_type) {
		case PIECE_TYPE_PAWN: {
			if (move->is_capture) {
				*to_write_to = move->source_file + 'a'; to_write_to++;
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
				}
				to_write_to++;
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
			*to_write_to = 'K'; to_write_to++;
			to_write_to = write_move_target_in_algebraic_notation_to_buf(move, to_write_to);
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


static char board_str_buf[512];
char *board_str(const struct board *board) {
	char *to_write_to = board_str_buf;

	for (int rank = 7; rank >= 0; rank--) {
		*to_write_to = rank + 1 + '0'; *to_write_to++;
		
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
	for (int rank = 7; rank >= 0; rank--) {
		for (int file = 0; file < 8; ) {
			char ch = *fen;

			if (ch >= '1' && ch <= '8') {
				int n_empty_squares = ch - '0';
				file += n_empty_squares;
			} else {
					
				struct square *current_square = &into->squares[rank][file];

				current_square->has_piece = true;
				
				switch (ch) {
				case 'p': case 'n': case 'b': case 'r': case 'q': case 'k':
					current_square->piece_color = PIECE_COLOR_BLACK;
					ch -= 32;
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
				case 'K':
				current_square->piece_type = PIECE_TYPE_KING; break;
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
}



// applies move to board without any constraints such as whose move it is, who is in check etc
void apply_move_to_board(struct board *board, const struct move *move) {
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
}

void undo_move_from_board(struct board *board, const struct move *move) {
	struct square *source_square = &board->squares[move->source_rank][move->source_file];
	struct square *target_square = &board->squares[move->target_rank][move->target_file];

	source_square->has_piece = true;
	source_square->piece_type = move->piece_type;
	source_square->piece_color = move->piece_color;

	if (move->is_capture) {
		target_square->piece_type = move->captured_piece_type;
		target_square->piece_color = invert_piece_color(move->piece_color);
	} else {
		target_square->has_piece = false;
	}
}

// returns whether the king located at [king_rank][king_file] is in check from the direction of rank_dir, file_dir
bool is_king_in_check_in_direction(const struct board *board, int king_rank, int king_file, int rank_dir, int file_dir) {
	piece_color king_color = board->squares[king_rank][king_file].piece_color;

	#define DIAGONAL_DIRECTION 0
	#define STRAIGHT_DIRECTION 1

	// are we looking along a diagonal or a file/rank?
	int direction_we_are_checking;
	if (rank_dir == 0 || file_dir == 0)
		direction_we_are_checking = STRAIGHT_DIRECTION;
	else
		direction_we_are_checking = DIAGONAL_DIRECTION;

	int rank = king_rank + rank_dir;
	int file = king_file + file_dir;
	while (1) {
		if (rank < 0 || rank > 7 || file < 0 || file > 7) // we went out of bounds
			return false;
		
		struct square the_square = board->squares[rank][file];
		if (the_square.has_piece) {
			if (the_square.piece_color == king_color) {
				// we've hit a piece of the king's color in this direction, that piece is pinned, and we are not in check from this direction
				return false;

			} else if (the_square.piece_type == PIECE_TYPE_QUEEN) {
				// the piece in this direction is a queen, which is checking the king regardless of whether our direction is diagonal or straight down a rank or file
				return true;
	
			} else if (the_square.piece_type == PIECE_TYPE_BISHOP && direction_we_are_checking == DIAGONAL_DIRECTION) {
				// the piece in this direction is a bishop, and we are looking in a diagonal direction, so the bishop is checking us
				return true;
			
			} else if (the_square.piece_type == PIECE_TYPE_ROOK && direction_we_are_checking == STRAIGHT_DIRECTION) {
				// the piece in this direction is a rook, and we are looking down a rank or file, so the rook is checking us
				return true;
			}
		}
		rank += rank_dir; file += file_dir;
	}
}

// returns whether the king located at king_rank, king_file is in check on the provided board
bool is_king_on_square_in_check(const struct board *board, int king_rank, int king_file) {
	piece_color king_color = board->squares[king_rank][king_file].piece_color;

	// strategy is to fan out from the king's position in all directions
	// and check if we hit a piece that attacks the king's square

	// pawn checks from the both of the king's sides
	{
		int pawn_rank, pawn_file;
		// check the square 1 diagonal left of the king
		if (king_color == PIECE_COLOR_WHITE) {
			pawn_rank = king_rank + 1;
			pawn_file = king_file - 1;
		} else {
			pawn_rank = king_rank - 1;
			pawn_file = king_file + 1;
		}

		if (pawn_rank >= 0 && pawn_rank <= 7 && pawn_file >= 0 && pawn_file <= 7) {
			struct square pawn_square = board->squares[pawn_rank][pawn_file];
			if (pawn_square.has_piece && pawn_square.piece_color != king_color && pawn_square.piece_type == PIECE_TYPE_PAWN)
				return true;
		}

		// check the square 1 diagonal right of the king
		if (king_color == PIECE_COLOR_WHITE) {
			pawn_rank = king_rank + 1;
			pawn_file = king_file + 1;
		} else {
			pawn_rank = king_rank - 1;
			pawn_file = king_file - 1;
		}
		if (pawn_rank >= 0 && pawn_rank <= 7 && pawn_file >= 0 && pawn_file <= 7) {
			struct square pawn_square = board->squares[pawn_rank][pawn_file];
			if (pawn_square.has_piece && pawn_square.piece_color != king_color && pawn_square.piece_type == PIECE_TYPE_PAWN)
				return true;
		}
	}

	// diagonal checks for bishop or queen checks
	{
		// -1, -1 direction
		if (is_king_in_check_in_direction(board, king_rank, king_file, -1, -1))
			return true;

		// -1, 1
		if (is_king_in_check_in_direction(board, king_rank, king_file, -1, 1))
			return true;

		// 1, -1 direction
		if (is_king_in_check_in_direction(board, king_rank, king_file, 1, -1))
			return true;

		// 1, 1 direction
		if (is_king_in_check_in_direction(board, king_rank, king_file, 1, 1))
			return true;
	}

	// checks along a rank or file by either a queen or rook
	{
		// 1, 0 direction
		if (is_king_in_check_in_direction(board, king_rank, king_file, 1, 0))
			return true;

		// -1, 0 direction
		if (is_king_in_check_in_direction(board, king_rank, king_file, -1, 0))
			return true;

		// 0, 1 direction
		if (is_king_in_check_in_direction(board, king_rank, king_file, 0, 1))
			return true;

		// 0, -1 direction
		if (is_king_in_check_in_direction(board, king_rank, king_file, 0, -1))
			return true;
	}
	
	// checks from a knight
	{
		for (int i = 0; i < 8; i++) {
			int rank = king_rank + knight_move_rank_offsets[i];
			int file = king_file + knight_move_file_offsets[i];
	
			if (rank < 0 || rank > 7 || file < 0 || file > 7)
				continue;

			struct square the_square = board->squares[rank][file];
			
			if (the_square.has_piece && the_square.piece_color != king_color && the_square.piece_type == PIECE_TYPE_KNIGHT)
				return true;
		}
	}

	return false;
}

// finds the position of the king of color king_color and puts the rank & file into *rank & *file
void find_king_position(const struct board *board, piece_color king_color, int *rank, int *file) {
	for (int r = 0; r < 8; r++) {
		for (int f = 0; f < 8; f++) {
			if (board->squares[r][f].has_piece && board->squares[r][f].piece_color == king_color && board->squares[r][f].piece_type == PIECE_TYPE_KING) {
				*rank = r;
				*file = f;
				return;
			} // rank loop
		} // file loop
	}

	fprintf(stderr, "unable to find kind of color %d on the board, board state:\n%s\n", king_color, board_str(board));
	exit(1);
}


bool is_move_legal(struct board *board, struct move *move) {
	apply_move_to_board(board, move);

	int king_rank, king_file;
	find_king_position(board, move->piece_color, &king_rank, &king_file);

	// a move is illegal if it puts the mover's side's king in check
	bool is_illegal = is_king_on_square_in_check(board, king_rank, king_file);
	
	undo_move_from_board(board, move);

	return !is_illegal;
}

int find_all_possible_pawn_moves(const struct board *board, struct move *into, int rank, int file) {
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
	assert(next_rank >= 0);
	assert(next_rank <= 7);
	
	int n_moves = 0;

	// since a move's origin square never changes, reuse this for populating move's fields
	struct move next_move = {0};
	next_move.source_rank = rank;
	next_move.source_file = file;
	next_move.piece_type = PIECE_TYPE_PAWN;
	next_move.piece_color = pawn_color;

	// move forward one square logic
	{
		struct square square_in_front_of_pawn = board->squares[next_rank][file];
		if (!square_in_front_of_pawn.has_piece) {
			next_move.target_rank = next_rank;
			next_move.target_file = file;
			next_move.is_capture = false;

			// promotion case for pawn
			if (next_rank == 0 || next_rank == 7) {
				next_move.is_promotion = true;
				piece_type possible_promotions[4] = { PIECE_TYPE_QUEEN, PIECE_TYPE_ROOK, PIECE_TYPE_BISHOP, PIECE_TYPE_KNIGHT };
				for (int i = 0; i < 4; i++) {
					next_move.piece_type_promoted_to = possible_promotions[i];
					*into = next_move;
					into++;
					n_moves++;
				}

			} else { // forward move without promotion
				next_move.is_promotion = false;
				*into = next_move; into++; n_moves++;
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
				next_move.is_promotion = false;
				next_move.target_rank = next_rank;
				next_move.target_file = left_file;
				next_move.captured_piece_type = target_square.piece_type;

				*into = next_move; into++; n_moves++;
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
		
			// we should never end up in situation where the target square has a king and the pawn can capture it
			assert(target_square.piece_type != PIECE_TYPE_KING);


			if (target_square.has_piece && target_square.piece_color != pawn_color) {
				next_move.is_capture = true;
				next_move.is_promotion = false;
				next_move.target_rank = next_rank;
				next_move.target_file = right_file;
				next_move.captured_piece_type = target_square.piece_type;

				*into = next_move; into++; n_moves++;
			}
		}
	}

	// forward 2 squares logic
	{
		int target_rank = - 1;
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
			*into = next_move; into++; n_moves++;
		}
		
	}

	return n_moves;
}



int find_all_possible_knight_moves(struct board *board, struct move *into, int rank, int file) {
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

			if (is_move_legal(board, &next_move)) {
				*into = next_move; into++; n_moves++;
			}
		}
	}

	return n_moves;
}

int find_all_possible_moves_in_direction(struct board *board, struct move *into, int rank, int file, int rank_dir, int file_dir) {
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
				if (is_move_legal(board, &next_move)) {
					*into = next_move; into++; n_moves++;
				}
			}
			break;
		} else {
			next_move.is_capture = false;
			if (is_move_legal(board, &next_move))
				*into = next_move; into++; n_moves++;
		}

		target_rank += rank_dir;
		target_file += file_dir;
	}

	return n_moves;
}

int find_all_possible_bishop_moves(struct board *board, struct move *into, int rank, int file) {
	assert(rank >= 0 && rank <= 7);
	assert(file >= 0 && rank <= 7);
	assert(board->squares[rank][file].has_piece);
	assert(board->squares[rank][file].piece_type == PIECE_TYPE_BISHOP);

	int n_moves = 0;

	// +1, +1 diagonal direction
	int moves_in_direction = find_all_possible_moves_in_direction(board, into, rank, file, 1, 1);
	n_moves += moves_in_direction; into += moves_in_direction;

	// -1, +1 diagonal direction
	moves_in_direction = find_all_possible_moves_in_direction(board, into, rank, file, -1, 1);
	n_moves += moves_in_direction; into += moves_in_direction;

	// -1, -1 diagonal direction
	moves_in_direction = find_all_possible_moves_in_direction(board, into, rank, file, -1, -1);
	n_moves += moves_in_direction; into += moves_in_direction;

	// +1, -1 diagonal direction
	moves_in_direction = find_all_possible_moves_in_direction(board, into, rank, file, 1, -1);
	n_moves += moves_in_direction;

	return n_moves;
}


int find_all_possible_rook_moves(struct board *board, struct move *into, int rank, int file) {
	assert(rank >= 0 && rank <= 7);
	assert(file >= 0 && rank <= 7);
	assert(board->squares[rank][file].has_piece);
	assert(board->squares[rank][file].piece_type == PIECE_TYPE_ROOK);

	int n_moves = 0;

	int moves_in_direction = find_all_possible_moves_in_direction(board, into, rank, file, 1, 0);
	n_moves += moves_in_direction; into += moves_in_direction;

	moves_in_direction = find_all_possible_moves_in_direction(board, into, rank, file, -1, 0);
	n_moves += moves_in_direction; into += moves_in_direction;
	
	moves_in_direction = find_all_possible_moves_in_direction(board, into, rank, file, 0, 1);
	n_moves += moves_in_direction; into += moves_in_direction;
	
	moves_in_direction = find_all_possible_moves_in_direction(board, into, rank, file, 0, -1);
	n_moves += moves_in_direction;

	return n_moves;
}

int find_all_possible_queen_moves(struct board *board, struct move *into, int rank, int file) {
	assert(rank >= 0 && rank <= 7);
	assert(file >= 0 && rank <= 7);
	assert(board->squares[rank][file].has_piece);
	assert(board->squares[rank][file].piece_type == PIECE_TYPE_QUEEN);

	int n_moves = 0;

	// towards rank 7
	int moves_in_direction = find_all_possible_moves_in_direction(board, into, rank, file, 1, 0);
	n_moves += moves_in_direction; into += moves_in_direction;

	// toward rank 0
	moves_in_direction = find_all_possible_moves_in_direction(board, into, rank, file, -1, 0);
	n_moves += moves_in_direction; into += moves_in_direction;
	
	// toward file 7
	moves_in_direction = find_all_possible_moves_in_direction(board, into, rank, file, 0, 1);
	n_moves += moves_in_direction; into += moves_in_direction;
	
	// toward file 0
	moves_in_direction = find_all_possible_moves_in_direction(board, into, rank, file, 0, -1);
	n_moves += moves_in_direction; into += moves_in_direction;

	// +1, +1 diagonal direction
	moves_in_direction = find_all_possible_moves_in_direction(board, into, rank, file, 1, 1);
	n_moves += moves_in_direction; into += moves_in_direction;

	// -1, +1 diagonal direction
	moves_in_direction = find_all_possible_moves_in_direction(board, into, rank, file, -1, 1);
	n_moves += moves_in_direction; into += moves_in_direction;

	// -1, -1 diagonal direction
	moves_in_direction = find_all_possible_moves_in_direction(board, into, rank, file, -1, -1);
	n_moves += moves_in_direction; into += moves_in_direction;

	// +1, -1 diagonal direction
	moves_in_direction = find_all_possible_moves_in_direction(board, into, rank, file, 1, -1);
	n_moves += moves_in_direction;

	return n_moves;
}

int find_all_possible_king_moves(struct board *board, struct move *into, int rank, int file) {
	assert(rank >= 0 && rank <= 7);
	assert(file >= 0 && rank <= 7);
	assert(board->squares[rank][file].has_piece);
	assert(board->squares[rank][file].piece_type == PIECE_TYPE_KING);

	piece_color king_color = board->squares[rank][file].piece_color;

	struct move next_move = {0};
	next_move.piece_type = PIECE_TYPE_KING;
	next_move.piece_color = king_color;
	next_move.source_rank = rank;
	next_move.source_file = file;

	int n_moves = 0;

	for (int i = 0; i < 8; i++) {
		int target_rank = rank + king_move_rank_offsets[i];
		int target_file = file + king_move_file_offsets[i];

		if (target_rank < 0 || target_rank > 7 || target_file < 0 || target_file > 7)
			continue;

		struct square target_square = board->squares[target_rank][target_file];

		next_move.target_rank = target_rank;
		next_move.target_file = target_file;

		if (target_square.has_piece) {
			if (target_square.piece_color != king_color) {
				next_move.is_capture = true;
				next_move.captured_piece_type = target_square.piece_type;

				if (is_move_legal(board, &next_move)) {
					*into = next_move; into++; n_moves++;
				}
			}
			continue;
		} else {
			next_move.is_capture = false;
			if (is_move_legal(board, &next_move)) {
				*into = next_move; into++; n_moves++;
			}
		}
	}
	
	return n_moves;
}


// places all possible moves into the into ptr, it must be large enough
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
					n_piece_moves = find_all_possible_pawn_moves(board, into, rank, file);
				}
				break;

				case PIECE_TYPE_KNIGHT: {
					n_piece_moves = find_all_possible_knight_moves(board, into, rank, file);
				};
				break;

				case PIECE_TYPE_BISHOP: {
					n_piece_moves = find_all_possible_bishop_moves(board, into, rank, file);
				};
				break;

				case PIECE_TYPE_ROOK: {
					n_piece_moves = find_all_possible_rook_moves(board, into, rank, file);
				};
				break;

				case PIECE_TYPE_QUEEN: {
					n_piece_moves = find_all_possible_queen_moves(board, into, rank, file);
				};
				break;

				case PIECE_TYPE_KING: {
					n_piece_moves = find_all_possible_king_moves(board, into, rank, file);
				};
				break;

				default:
					assert(false);
			}
			n_moves += n_piece_moves;
			into += n_piece_moves;
		}
	}

	return n_moves;
}

int main(void) {
	struct board board;
	memset(board.squares, 0, 8 * 8 * sizeof(board.squares[0][0]));
	
	//char *starting_position_fen = "rnbqkbnr/pppppppp/8/8/4R3/8/PPPPPPPP/RNBQKBNR w KQkq - 0 1";
	char *position_fen = "r1b2rk1/p1nq2bp/8/3p4/1N2p3/1PN3P1/P2P2BP/R2Q1RK1 b - - 1 18";
	char *pinned_knight = "rnbqk1nr/pppp1ppp/8/4p3/1b1P4/2N5/PPP1PPPP/R1BQKBNR w KQkq - 2 3";

	load_fen_to_board(pinned_knight, &board);

	struct move moves[256];

	printf("%s\n\n", board_str(&board));

	
	int n_moves = find_all_possible_moves_for_color(&board, moves, PIECE_COLOR_WHITE);
	printf("n moves: %d\n", n_moves);
	for (int i = 0; i < n_moves; i++) {
		printf("%s\n", move_str(&moves[i]));
	}
}