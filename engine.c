#include <assert.h>
#include <stdio.h>
#include <stdlib.h>
#include <time.h>

#include "engine.h"
#include "chess.h"
#include "chess_utils.h"

void init_engine(void) {
	srand((unsigned int)(time(NULL)));
}

struct move find_best_move_for_color(struct position *the_position, bool is_piece_white) {
	struct move all_legal_moves[256];
	
	int n_legal_moves = find_all_possible_moves_for_color(the_position, all_legal_moves, is_piece_white);
	
	// this function should not have been called if the engine doesn't have a best move to give
	// having 0 legal moves means the game is over and the engine is mated
	assert(n_legal_moves > 0); 
	
	int best_move_idx = rand() % n_legal_moves;
	
	struct move engine_move = all_legal_moves[best_move_idx];
	
	fprintf(stderr, "%d legal moves for engine, chose move %d\n", n_legal_moves, best_move_idx);

	return engine_move;
}