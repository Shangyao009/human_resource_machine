#include <iostream>
#include <vector>
#include <list>
#include <string>
#include "playLevel.h"
using namespace std;

const int SCREEN_LEN = 100;
const int SCREEN_HEIGHT = 20;
const int EMPTY_MARKER = -2147483648;
vector<string> screen;

void initScreen()
{
    screen.resize(SCREEN_HEIGHT);
    for (int i = 0; i < SCREEN_HEIGHT; i++)
    {
        screen[i].resize(SCREEN_LEN + 1);
        for (int j = 0; j < SCREEN_LEN; j++)
        {
            screen[i][j] = ' ';
        }
        screen[i][SCREEN_LEN] = 0;
    }
}

void resetScreen()
{
    for (int i = 0; i < SCREEN_HEIGHT; i++)
    {
        for (int j = 0; j < SCREEN_LEN; j++)
        {
            screen[i][j] = ' ';
        }
    }
}

const int BOX_HEIGHT = 3;
const int BOX_WIDTH = 5;

// if invalid will draw 'X' within the box
void drawBox(int r, int c, int data, bool invalid = false)
{
    int fr = r;
    int fc = c;
    int lr = r + BOX_HEIGHT - 1;
    int lc = c + BOX_WIDTH - 1;
    int mr = (fr + lr) / 2;
    int mc = (fc + lc) / 2;
    screen[fr][fc] = screen[fr][lc] = '+';
    screen[lr][fc] = screen[lr][lc] = '+';
    for (int c = fc + 1; c < lc; c++)
        screen[fr][c] = screen[lr][c] = '-';
    for (int r = fr + 1; r < lr; r++)
        screen[r][fc] = screen[r][lc] = '|';
    if (invalid)
    {
        screen[mr][mc] = 'X';
        return;
    }

    if (data < 0)
    {
        screen[mr][mc - 1] = '-';
        data *= -1;
    }
    string txt = to_string(data);
    if (txt.length() >= 2)
    {
        screen[mr][mc] = txt[0];
        screen[mr][mc + 1] = txt[1];
    }
    else
        screen[mr][mc] = txt[0];
}

void drawBoxesVertical(int c, list<int> &data_list, int n_max = 6)
{
    list<int>::iterator it;
    int i = 0;
    for (it = data_list.begin(); it != data_list.end(); ++it)
    {
        int data = *it;
        int r = i * BOX_HEIGHT;

        if (data == EMPTY_MARKER)
            drawBox(r, c, 0, true);
        else
            drawBox(r, c, data);

        i++;
        if (i >= n_max)
            break;
    }
    for (; i < n_max; i++)
    {
        int r = i * BOX_HEIGHT;
        drawBox(r, c, 0, true);
    }
}

void drawPlaygroundBoxes(int sr, int sc, list<int> &data_list, int n_max = 4)
{
    list<int>::iterator it;
    int i = 0;
    for (it = data_list.begin(); it != data_list.end(); ++it)
    {
        int data = *it;
        int c = sc + i * (BOX_WIDTH + 1);
        if (data == EMPTY_MARKER)
            drawBox(sr, c, 0, true);
        else
            drawBox(sr, c, data);

        // draw index of playground boxes
        screen[sr + BOX_HEIGHT][c + (BOX_WIDTH - 1) / 2] = to_string(i)[0];

        i++;
        if (i >= n_max)
            break;
    }
    for (; i < n_max; i++)
    {
        int c = sc + i * (BOX_WIDTH + 1);
        drawBox(sr, c, 0, true);
        screen[sr + BOX_HEIGHT][c + (BOX_WIDTH - 1) / 2] = to_string(i)[0];
    }
}

void drawRobot(int r, int c, int box_data, bool box_taken)
{
    screen[r][c + BOX_WIDTH - 1] = screen[r][c] = '@';
    for (int i = 0; i < BOX_WIDTH; i++)
        screen[r + 1][c + i] = '-';
    screen[r + 2][c + BOX_WIDTH - 1] = screen[r + 2][c] = '|';
    screen[r + 2][c + BOX_WIDTH - 2] = screen[r + 2][c + 1] = '@';
    screen[r + 3][c + (BOX_WIDTH - 1) / 2] = '+';
    screen[r + 4][c] = '/';
    screen[r + 4][c + BOX_WIDTH - 1] = '\\';
    screen[r + 5][c + BOX_WIDTH - 2] = screen[r + 5][c + 1] = '|';
    if (!box_taken)
        return;
    drawBox(r - BOX_HEIGHT, c, box_data);
}

const int SPLIT_SCREEN_COLUMN = SCREEN_LEN * 7 / 10;

void drawLeftPanal(vector<string> &command, int current_line = -1, int MAX_LINE = 5)
{
    for (int i = 0; i < SCREEN_HEIGHT; i++)
    {
        screen[i][SPLIT_SCREEN_COLUMN] = '|';
    }
    for (int i = 0; i < 5; i++)
    {
        screen[0][SPLIT_SCREEN_COLUMN + 2 + i] = '=';
    }
    screen[0][SPLIT_SCREEN_COLUMN + 8] = 'C';
    screen[0][SPLIT_SCREEN_COLUMN + 9] = 'O';
    screen[0][SPLIT_SCREEN_COLUMN + 10] = 'D';
    screen[0][SPLIT_SCREEN_COLUMN + 11] = 'E';

    for (int i = 1; i < 5; i++)
    {
        screen[0][SPLIT_SCREEN_COLUMN + 12 + i] = '=';
    }

    int _c = SPLIT_SCREEN_COLUMN + 4;
    int _r = 1;
    int l = command.size();
    int i = l > MAX_LINE ? l - MAX_LINE : 0;
    if (current_line > 0)
    {
        if (current_line + MAX_LINE <= l)
            i = current_line - 1;
        else
            i = l - MAX_LINE;
        if (i < 0)
            i = 0;
    }
    l = min(l, i + MAX_LINE);
    for (; i < l; i++)
    {
        string code = command[i];

        // print line number
        string line = to_string(i + 1);
        if (line.length() >= 2)
        {
            screen[_r][_c - 1] = line[0];
            screen[_r][_c] = line[1];
        }
        else
            screen[_r][_c] = line[0];

        for (int j = 0; j < code.length(); j++)
            screen[_r][_c + 2 + j] = code[j];

        if ((i + 1) == current_line)
            screen[_r][_c - 2] = '>';
        _r++;
    }
}

void drawScreen(list<int> &in,list<int> out, list<int>&playground, vector<string> &command)
{
    // draw In Boxes
    drawBoxesVertical(BOX_WIDTH + 1, in); // 1 for the padding
    // draw Out Boxes
    drawBoxesVertical(8 * (BOX_WIDTH + 1), out);

    // draw playground Boxes
    drawPlaygroundBoxes(3.5f * BOX_HEIGHT, 3 * (BOX_WIDTH + 1), playground);
    drawRobot(BOX_HEIGHT, 3 * (BOX_WIDTH + 1), 5, true);

    drawLeftPanal(command, 2);
}

void printScreen()
{
    for (string s : screen)
    {
        cout << s << endl;
    }
}
