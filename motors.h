#define FULL_TURN 200

typedef struct Coil {
    unsigned char *port;
    unsigned bit;
} Coil;

typedef Coil *pCoil;

typedef struct StepMotor {
    Coil coils[4];
    unsigned long delay;
} StepMotor;

typedef StepMotor *pMotor;

void moveOneStep(pMotor motor, unsigned seq[4][4]);

void moveLeft(pMotor motor);

void moveRight(pMotor motor);

void dePower(pMotor motor);

void initMotor(pMotor motor);

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


