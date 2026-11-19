void cbLoadLevel(int level) {
  cbBrickCount = 0;

  for (int r = 0; r < CB_ROWS; r++) {
    for (int c = 0; c < CB_COLS; c++) {
      cbBricks[r][c].alive = false;
      cbBricks[r][c].alien = false;
      cbBricks[r][c].hp = 0;
    }
  }

  if (level % 3 == 0) {
    // Niveau aliens style Space Invader
    for (int r = 0; r < 4; r++) {
      for (int c = 1; c < CB_COLS - 1; c++) {
        if ((r + c) % 2 == 0 || r == 1) {
          cbBricks[r][c].alive = true;
          cbBricks[r][c].alien = true;
          cbBricks[r][c].hp = 1 + (level / 3);
          cbBrickCount++;
        }
      }
    }
  }

  else if (level % 3 == 1) {
    // Mur complet dégradé
    for (int r = 0; r < 5; r++) {
      for (int c = 0; c < CB_COLS; c++) {
        cbBricks[r][c].alive = true;
        cbBricks[r][c].hp = 1 + (r / 2);
        cbBrickCount++;
      }
    }
  }

  else {
    // Motif tunnel
    for (int r = 0; r < 6; r++) {
      for (int c = 0; c < CB_COLS; c++) {
        if (c == r || c == CB_COLS - 1 - r || r < 2) {
          cbBricks[r][c].alive = true;
          cbBricks[r][c].hp = 1;
          cbBrickCount++;
        }
      }
    }
  }
}