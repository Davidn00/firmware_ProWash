#define FULL_TURN 200

unsigned sequenceRight[4][4] = {
        {1, 0, 0, 0},
        {0, 1, 0, 0},
        {0, 0, 1, 0},
        {0, 0, 0, 1},
};

unsigned sequenceLeft[4][4] = {
        {0, 0, 0, 1},
        {0, 0, 1, 0},
        {0, 1, 0, 0},
        {1, 0, 0, 0},
};


void moveOneStep(pMotor motor, unsigned sequence[4][4]) {
    for (int i = 0; i != 4; i++) {
        for (int j = 0; j != 4; j++) {
            if (sequence[i][j]) {
                *(motor->coils[j].port) |= (1 << motor->coils[j].bit);
            } else {
                *(motor->coils[j].port) &= ~(1 << motor->coils[j].bit);
            }
        }
        delay_ms(motor->delay);
    }
}

void moveLeft(pMotor motor) {
    moveOneStep(motor, sequenceLeft);
}

void moveRight(pMotor motor) {
    moveOneStep(motor, sequenceRight);
}

void dePower(pMotor motor) {
    for (int i = 0; i != 4; i++) {
        *(motor->coils[i].port) &= ~(1 << motor->coils[i].bit);
    }
}

void initMotor(pMotor motor) {
    dePower(motor);
    motor->delay = 5;
}

   


