// PONG 1D //
// jeu de pong led en une dimension (celle de la bande led).
// TIPS de pro : pour plus de fun multiplier les Pong 1D pour avoir plusieurs bandes leds simultanées : éclate garantie
// Ce code est un remix du code de 2020 de FlyingAngel (source : https://projecthub.arduino.cc/flyingangel/1d-pong-1a0acf)

#define FASTLED_INTERNAL       // Disable version number message in FastLED library (looks like an error)
#include <FastLED.h>

// *********************************
// PARAMETRES DU JEU
// *********************************

#define NUM_LEDS 94             // Nombre total de LEDs
#define DATA_PIN 3              // Pin utilisée pour contrôler les LEDs

int maxBright = 155;            // Luminosité maximale des LEDs

byte playerBtnPin[] = {5, 8};   // Pins utilisées pour les boutons des joueurs
byte playerLedPin[] = {6, 9};   // Pins utilisées pour les LEDs des boutons

int gameSpeedMin = 40;          // Vitesse minimale du jeu
int gameSpeedMax = 4;           // Vitesse maximale (valeur faible = rapide)
int gameSpeedStep = 4;          // Accélération à chaque renvoi
int ballSpeedMax = 1;           // Vitesse minimale que peut atteindre la balle
int ballBoost0 = 30;            // Bonus de vitesse si la balle est au bout
int ballBoost1 = 20;            // Bonus de vitesse si la balle est presque au bout
byte playerColor[] = {0, 96};   // Couleurs des joueurs (rouge et vert)
int winRounds = 6;              // Nombre de points pour gagner
int endZoneSize = 5;            // Taille de la zone de fin
int endZoneColor = 160;         // Couleur des zones de fin (bleu/vert)
int reloadButton = 10;          // Délai pour la remise à zéro du bouton


// *********************************
// VARIBALE DU JEU NON MODIFIABLE
// *********************************

boolean activeGame = false;                 // Le jeu est-il actif ?
unsigned long previousMoveMillis;           // Dernier moment où la balle a bougé
unsigned long previousButtonMillis;         // Dernier moment où un bouton a été pressé
int playerButtonPressed[2];                 // Position où le joueur a appuyé
int previousButtonPos = -1;                 // Dernière position d'appui
byte previousButtonColor;                   // Couleur du champ où le bouton a été pressé
int playerScore[2];                         // Score de chaque joueur
byte playerStart;                           // Quel joueur commence ?
int gameSpeed;                              // Vitesse actuelle du jeu
int ballDir = 1;                            // Direction de la balle (1 ou -1)
int ballPos;                                // Position actuelle de la balle
int ballSpeed;                              // Vitesse actuelle de la balle
int ballMove;                               // Nombre de déplacements effectués

CRGB leds[NUM_LEDS];                        // Tableau représentant toutes les LEDs
byte previousButtonBright = maxBright / 2;  // Luminosité de l'ancienne position d'appui
byte scoreDimBright = maxBright / 4;        // Luminosité des scores en version atténuée



// *********************************
// INITIALISATION
// *********************************
void setup()
{
  randomSeed(analogRead(0));          // génartion du hasard

  // FastLed definition
  FastLED.addLeds<NEOPIXEL, DATA_PIN>(leds, NUM_LEDS);

  // PIN defination
  pinMode(playerBtnPin[0], INPUT_PULLUP);   // Délcaration des pins boutons
  pinMode(playerBtnPin[1], INPUT_PULLUP);

  pinMode(playerLedPin[0], OUTPUT);
  pinMode(playerLedPin[1], OUTPUT);         // Délcaration des pins led

  playerStart = random(2);  // commencer par un joueur au hasard
}

void loop()
{
  GeneratePlayField(maxBright);
  FastLED.show();
  activeGame = true;
  game();
}

// function to debounce button (true == pressed, false == not pressed)
boolean buttonBounce(byte button, byte bounceTime) // bounce the button
{
  boolean result = false;

  if (digitalRead(playerBtnPin[button]) == LOW)
  {
    delay (bounceTime);
    if (digitalRead(playerBtnPin[button]) == LOW)
    {
      result = true;
    }
  }
  return result;
}


void game()
{
  while (activeGame)
  {

    gameSpeed = gameSpeedMin;           // set starting game speed
    ballSpeed = gameSpeed;              // set starting ball speed
    memset(playerButtonPressed, -1, sizeof(playerButtonPressed)); // clear keypress

    GeneratePlayField(scoreDimBright);  // show gamefield with dimmed score
    FastLED.show();

    InitializePlayers();  // set the player-settings -> wait for keypress to start game

    GameLoop();           // main loop: move ball -> ball left gamefield? -> check keypress -> check if keypress in endzone -> change direction

    CheckScore();         // check who made score and show it

    CheckWinner();        // check if we have a winner
  }
}


void InitializePlayers()
{

  if (playerStart == 0)   // initialize for player 0
  {
    ballDir = 1;          // set ball direction
    ballPos = 0;          // set startposition of ball

    digitalWrite(playerLedPin[0], HIGH);              // activate LED
    while (digitalRead(playerBtnPin[0]) == HIGH) {} // wait for button
    digitalWrite(playerLedPin[0], LOW);               // deactivate LED
  }
  else                        // initialize for player 1
  {
    ballDir = -1;             // set ball direction
    ballPos = NUM_LEDS - 1;   // set startposition of ball

    digitalWrite(playerLedPin[1], HIGH);              // activate LED
    while (digitalRead(playerBtnPin[1]) == HIGH) {} // wait for button
    digitalWrite(playerLedPin[1], LOW);               // deactivate LED
  }
}


void GameLoop()
{
  while (true)    // loop, exit with break when one player made a score
  {

    if (millis() - previousMoveMillis > ballSpeed)  // time to move ball
    {
      previousMoveMillis = millis();

      GeneratePlayField(scoreDimBright);

      ballMove++;
      if (ballMove >= (reloadButton*endZoneSize))
      {
        ballMove = 0;
        memset(playerButtonPressed, -1, sizeof(playerButtonPressed));                // leave loop -> one player made a score
      }

      ballPos += ballDir;
      if (ballPos < 0 || ballPos >= NUM_LEDS) // ball left endzone?
      {
        break;                // leave loop -> one player made a score
      }


      // ici pour compter le nombre de "pas" de la ball (réinitialiser la ballpos si le nombre de pas est ok)

      leds[ballPos] = CHSV(0, 0, maxBright);    // generate ball (white)
      FastLED.show();
    }

    CheckButtons();     // check keypress

    // fix positions of keypress for testing
    // if (ballPos == 3) playerButtonPressed[0] = 3;
    // if (ballPos == 59) playerButtonPressed[1] = 59;

    CheckButtonPressedPosition();

  } // end of while-loop
}


// *** check if buttons pressed
void CheckButtons()
{
  for (int i = 0; i < 2; i++)
  {
    // player pressed button?
    if (playerButtonPressed[i] == -1 && digitalRead(playerBtnPin[i]) == LOW && (ballDir + 1) / 2 == i)
      // (ballDir + 1) / 2 == i  -->  TRUE, when:
      // ballDir == -1  AND  i = 0  -->  player 0 is active player
      // ballDir == +1  AND  i = 1  -->  player 1 is active player
      // only the button-press of the active player is stored
    {
      playerButtonPressed[i] = ballPos;   //store position of pressed button
      previousButtonPos = ballPos;
      previousButtonColor = playerColor[i];
      previousButtonMillis = millis(); // store time when button was pressed
    }
  }
}


// *** check, if button was pressed when ball was in endzone and if so, change direction of ball
void CheckButtonPressedPosition()
{
  if (ballDir == -1 && playerButtonPressed[0] <= endZoneSize - 1 && playerButtonPressed[0] != -1)
  {
    ChangeDirection();
  }

  if (ballDir == +1 && playerButtonPressed[1] >= NUM_LEDS - endZoneSize)
  {
    ChangeDirection();
  }

  /*

    if (ballDir == +1 && playerButtonPressed[1] <= NUM_LEDS - endZoneSize*3)
    {
    memset(playerButtonPressed, -1, sizeof(playerButtonPressed));
    }

    if (ballDir == -1 && playerButtonPressed[0] <= NUM_LEDS - endZoneSize*3)
    {
    memset(playerButtonPressed, -1, sizeof(playerButtonPressed));
    }

  */

  // réinitialiser la sauvegarde bouton si le bouton a été appuyé vraiment trop tot
  // memset(playerButtonPressed, -1, sizeof(playerButtonPressed));

  // sinon faire dans l'animation de la balle un compteur qui rechare la ballpos après X x de endzone

}


void ChangeDirection()
{
  ballDir *= -1;
  gameSpeed -= gameSpeedStep;
  ballSpeed = gameSpeed;
  if (ballPos == 0 || ballPos == NUM_LEDS - 1)  // triggered on first or last segment
  {
    ballSpeed -= ballBoost0;      // Super-Boost
  }

  if (ballPos == 1 || ballPos == NUM_LEDS - 2)  // triggered on second or forelast segment
  {
    ballSpeed -= ballBoost1;      // Boost
  }

  ballSpeed = max(ballSpeed, ballSpeedMax);                     // limit the maximum ballspeed
  memset(playerButtonPressed, -1, sizeof(playerButtonPressed)); // clear keypress
}


void CheckScore()
{
  previousButtonPos = -1;     // clear last ball-position at button-press

  // check who made score
  if (ballPos < 0)            // player1 made the score
  {
    playerScore[1] += 1;      // new score for player1

    GeneratePlayField(maxBright);   // show new score full bright
    BlinkNewScore(NUM_LEDS / 2 - 1 + playerScore[1], playerColor[1]); // blink last score

    playerStart = 1;          // define next player to start (player, who made the point)
  }
  else                        // player0 made the score
  {
    playerScore[0] += 1;      // new score for player0

    GeneratePlayField(maxBright);   // show new score full bright
    BlinkNewScore(NUM_LEDS / 2 - playerScore[0], playerColor[0]); // blink last score

    playerStart = 0;          // define next player to start (player, who made the point)
  }

  GeneratePlayField(maxBright);     // show new score full bright
  FastLED.show();

  delay(1000);
}


void CheckWinner()
{
  // check if we have a winner
  if (playerScore[0] >= winRounds || playerScore[1] >= winRounds)
  { // we have a winner!

    activeGame = false;

    FastLED.clear();
    Rainbow(playerScore[0] > playerScore[1]); // TRUE if player 0 won; FALSE when player 1 won

    memset(playerScore, 0, sizeof(playerScore));  // reset all scores

    playerStart = abs(playerStart - 1);   // next game starts looser

  }
}


void GeneratePlayField(byte bright)
{
  FastLED.clear();        // clear all
  GenerateEndZone();      // generate endzone
  GenerateScore(bright);  // generate actual score
  GenerateLastHit();      // generate mark of position of last button-press
}


void GenerateEndZone()
{
  for (int i = 0; i < endZoneSize; i++)
  {
    leds[i] = CHSV(endZoneColor, 255, maxBright);
    leds[NUM_LEDS - 1 - i] = CHSV(endZoneColor, 255, maxBright);
  }
}


void GenerateScore(int bright)
{
  int i;

  // Player 0
  for (i = 0; i < playerScore[0]; i++)
  {
    leds[NUM_LEDS / 2 - 1 - i] = CHSV(playerColor[0], 255, bright);
  }

  // Player 1
  for (i = 0; i < playerScore[1]; i++)
  {
    leds[NUM_LEDS / 2 + i] = CHSV(playerColor[1], 255, bright);
  }
}


void GenerateLastHit()
{
  if (previousButtonPos != -1 && previousButtonMillis + 500 > millis())
  {
    leds[previousButtonPos] = CHSV(previousButtonColor, 255, previousButtonBright);
  }
}


void BlinkNewScore(int pos, byte color)
{
  for (int i = 1; i <= 4; i++)
  {
    leds[pos] = CHSV(color, 255, (i % 2) * maxBright);  // blink LED 2 times (1-0-1-0)
    FastLED.show();
    delay(300);
  }
}


void Rainbow(boolean won)
{
  for (int k = 0; k < 3; k++)   // 3 rounds rainbow
  {
    for (int j = 0; j <= 255; j++)
    {
      for (int i = 0; i < NUM_LEDS / 2; i++)
      {
        if (won == true)    // player 0 won --> Rainbow left
        {
          leds[i] = CHSV(((i * 256 / NUM_LEDS) + j) % 256, 255, maxBright);
        }
        else                // player 1 won --> Rainbow right
        {
          leds[NUM_LEDS - i - 1] = CHSV(((i * 256 / NUM_LEDS) + j) % 256, 255, maxBright);
        }
      }
      FastLED.show();
      delay(7);
    }
  }
}
