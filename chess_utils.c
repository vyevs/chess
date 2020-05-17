#include <assert.h>
#include <stdio.h>
#include <string.h>
#include <stdlib.h>

#include "chess.h"
#include "chess_utils.h"

static char *write_move_target_in_algebraic_notation_to_buf(const struct move *move, char *to_write_to) {
	// only write the source square if the moved piece is not a king, since there can only be 1 king for each color
	if (move->piece_type != PIECE_TYPE_KING) {
		*to_write_to = move->source_file + 'a'; to_write_to++;
		*to_write_to = move->source_rank + '1'; to_write_to++;
	}
	if (move->is_capture) {
		*to_write_to = 'x'; to_write_to++;
	}
	*to_write_to = move->target_file + 'a'; to_write_to++;
	*to_write_to = move->target_rank + '1'; to_write_to++;
	return to_write_to;
}


char *move_str(const struct move *move) {
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



char *position_str(const struct position *position) {
	static char position_str_buf[512];
	char *to_write_to = position_str_buf;

	for (int rank = 7; rank >= 0; rank--) {
		*to_write_to = rank + 1 + '0'; to_write_to++;
		
		*to_write_to = ' '; to_write_to++;
		*to_write_to = ' '; to_write_to++;

		for (int file = 0; file < 8; file++) {
			struct square square = position->squares[rank][file];
			
			if (!square.has_piece) {
				*to_write_to = ' '; to_write_to++;
				continue;
			}

			piece_type piece_type = square.piece_type;
			bool is_piece_white = square.is_piece_white;
			
			char piece_ch;
			switch (square.piece_type) {
				case PIECE_TYPE_PAWN: piece_ch = 'P'; break;
				case PIECE_TYPE_KNIGHT: piece_ch = 'N'; break;
				case PIECE_TYPE_BISHOP: piece_ch = 'B'; break;
				case PIECE_TYPE_ROOK: piece_ch = 'R'; break;
				case PIECE_TYPE_QUEEN: piece_ch = 'Q'; break;
				case PIECE_TYPE_KING: piece_ch = 'K'; break;

				default: {
					fprintf(stderr, "position_str: unknown piece type %d at [%d, %d]\n", piece_type, rank, file);
					exit(1);
				};
			}
			
			if (!is_piece_white)
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

	return position_str_buf;
}

void load_fen_to_position(const char *fen, struct position *into) {
	into->white_king_rank = -1;
	into->white_king_file = -1;
	into->black_king_rank = -1;
	into->black_king_file = -1;

	for (int rank = 7; rank >= 0; rank--) {
		for (int file = 0; file < 8; ) {
			char ch = *fen;

			if (ch >= '1' && ch <= '8') {
				int n_empty_squares = ch - '0';
				
				for (int empty_sq = 0; empty_sq < n_empty_squares; empty_sq++) {
					into->squares[rank][file].has_piece = false;
					
					file++;
				}
				
			} else {
					
				struct square *current_square = &into->squares[rank][file];

				current_square->has_piece = true;
				
				if (ch == 'p' || ch == 'n' || ch == 'b' || ch == 'r' || ch == 'q' || ch == 'k') {
					current_square->is_piece_white = false;
					ch -= 32;
				} else {
					current_square->is_piece_white = true;
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
					if (current_square->is_piece_white) {
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