#include <iostream>
#include <vector>
#include <list>
#include <string>
#include <chrono>
#include <thread>
#include <algorithm>
#include <windows.h>
#include <fstream>

using namespace std;

enum class CommandId
{
    inbox,
    outbox,
    add,
    sub,
    copyto,
    copyfrom,
    jump,
    jumpifzero,
    invalid,
};

CommandId parseStr(string &s)
{
    if (s.compare("inbox") == 0)
        return CommandId::inbox;
    if (s.compare("outbox") == 0)
        return CommandId::outbox;
    if (s.compare("add") == 0)
        return CommandId::add;
    if (s.compare("sub") == 0)
        return CommandId::sub;
    if (s.compare("copyto") == 0)
        return CommandId::copyto;
    if (s.compare("copyfrom") == 0)
        return CommandId::copyfrom;
    if (s.compare("jump") == 0)
        return CommandId::jump;
    if (s.compare("jumpifzero") == 0)
        return CommandId::jumpifzero;
    return CommandId::invalid;
}

string toStr(CommandId id)
{
    switch (id)
    {
    case CommandId::inbox:
        return "inbox";
    case CommandId::outbox:
        return "outbox";
    case CommandId::add:
        return "add";
    case CommandId::sub:
        return "sub";
    case CommandId::copyto:
        return "copyto";
    case CommandId::copyfrom:
        return "copyfrom";
    case CommandId::jump:
        return "jump";
    case CommandId::jumpifzero:
        return "jumpifzero";
    default:
        return "";
    }
}

class Command
{
public:
    CommandId id;
    int arg;
    bool no_args;
    Command(string str, int arg) : arg(arg), no_args(false) { id = parseStr(str); }
    Command(string str) : arg(0), no_args(true) { id = parseStr(str); }
};

class Box
{
public:
    int data;
    bool isEmpty;
    Box(int data) : data(data), isEmpty(false) {}
    Box() : data(0), isEmpty(true) {}
    Box(const Box &other) : data(other.data), isEmpty(other.isEmpty) {}

    void empty()
    {
        data = 0;
        isEmpty = true;
    }

    bool isZero()
    {
        return !isEmpty && data == 0;
    }

    void copyBox(Box &src)
    {
        data = src.data;
        isEmpty = src.isEmpty;
    }

    void addBox(Box &src)
    {
        data += src.data;
        isEmpty = src.isEmpty;
    }

    void subBox(Box &src)
    {
        data -= src.data;
        isEmpty = src.isEmpty;
    }
};

class GameScreen
{
    const int SCREEN_LEN = 100;
    const int SCREEN_HEIGHT = 20;
    const int BOX_HEIGHT = 3;
    const int BOX_WIDTH = 5;
    const int SPLIT_SCREEN_COLUMN = SCREEN_LEN * 7 / 10;
    vector<string> screen;

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

    void drawBoxesVertical(int c, list<Box> &boxes, int n_max)
    {
        list<Box>::iterator it;
        int i = 0;
        for (it = boxes.begin(); it != boxes.end(); ++it)
        {
            int data = (*it).data;
            int r = i * BOX_HEIGHT;

            if ((*it).isEmpty)
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

    void drawPlaygroundBoxes(int sr, int sc, vector<Box> &boxes, int n_box)
    {
        int i = 0;
        for (; i < boxes.size(); i++)
        {
            Box &box = boxes[i];
            int data = box.data;
            int c = sc + i * (BOX_WIDTH + 1);
            if (box.isEmpty)
                drawBox(sr, c, 0, true);
            else
                drawBox(sr, c, data);

            // draw index of playground boxes
            screen[sr + BOX_HEIGHT][c + (BOX_WIDTH - 1) / 2] = to_string(i)[0];

            if (i >= n_box)
                break;
        }
        for (; i < n_box; i++)
        {
            int c = sc + i * (BOX_WIDTH + 1);
            drawBox(sr, c, 0, true);
            screen[sr + BOX_HEIGHT][c + (BOX_WIDTH - 1) / 2] = to_string(i)[0];
        }
    }

    void drawRobot(int r, int c, Box &box_taken)
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
        if (box_taken.isEmpty)
            return;
        drawBox(r - BOX_HEIGHT, c, box_taken.data);
    }

    void drawSeperateLine()
    {
        for (int i = 0; i < SCREEN_HEIGHT; i++)
        {
            screen[i][SPLIT_SCREEN_COLUMN] = '|';
        }
    }

    void drawCodeBlock(vector<Command> &code, int current_line = -1, int MAX_LINE = 5)
    {
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
        int l = code.size();
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
            string command = toStr(code[i].id);
            if (!code[i].no_args)
                command += " " + to_string(code[i].arg);
            // print line number
            string line = to_string(i + 1);
            if (line.length() >= 2)
            {
                screen[_r][_c - 1] = line[0];
                screen[_r][_c] = line[1];
            }
            else
                screen[_r][_c] = line[0];

            for (int j = 0; j < command.length(); j++)
                screen[_r][_c + 2 + j] = command[j];

            if ((i + 1) == current_line)
                screen[_r][_c - 2] = '>';
            _r++;
        }
    }

public:
    GameScreen()
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

    void draw(list<Box> &in, list<Box> &out, vector<Box> &playground, vector<Command> &code, int current_line, int robot_column, Box &box_taken)
    {
        // draw In Boxes
        drawBoxesVertical(BOX_WIDTH + 1, in, 6);
        // draw out boxes
        drawBoxesVertical(8 * (BOX_WIDTH + 1), out, 6);

        drawPlaygroundBoxes(3.5f * BOX_HEIGHT, 3 * (BOX_WIDTH + 1), playground, playground.size());
        drawRobot(BOX_HEIGHT, robot_column * (BOX_WIDTH + 1), box_taken);

        drawSeperateLine();
        drawCodeBlock(code, current_line, 10);
    }

    void print()
    {
        for (string s : screen)
        {
            cout << s << endl;
        }
    }

    void clear()
    {
        for (int i = 0; i < SCREEN_HEIGHT; i++)
        {
            for (int j = 0; j < SCREEN_LEN; j++)
            {
                screen[i][j] = ' ';
            }
        }
    }
};

void delay(int ms)
{
    Sleep(ms);
}

void hideCursor()
{
    HANDLE hConsole = GetStdHandle(STD_OUTPUT_HANDLE);
    CONSOLE_CURSOR_INFO cursorInfo;
    GetConsoleCursorInfo(hConsole, &cursorInfo);
    cursorInfo.bVisible = FALSE; // Set visibility to false
    SetConsoleCursorInfo(hConsole, &cursorInfo);
}

void showCursor()
{
    HANDLE hConsole = GetStdHandle(STD_OUTPUT_HANDLE);
    CONSOLE_CURSOR_INFO cursorInfo;
    GetConsoleCursorInfo(hConsole, &cursorInfo);
    cursorInfo.bVisible = TRUE; // Set visibility to true
    SetConsoleCursorInfo(hConsole, &cursorInfo);
}

void cleanLineAbove()
{
    // Move up one line
    cout << "\033[A";
    // Clear the line
    cout << "\033[K";
}

class Game
{
    void robot_move_left()
    {
        robot_column -= 1;
    }

    void robot_move_right()
    {
        robot_column += 1;
    }

    void goToPlayGround(int x)
    {
        int target_column = 3 + x - 1;
        while (robot_column < target_column)
        {
            delay(step_delay);
            robot_move_right();
            updateScreen();
        }
        while (robot_column > target_column)
        {
            delay(step_delay);
            robot_move_left();
            updateScreen();
        }
    }

    void goToInBox()
    {
        while (robot_column > 3)
        {
            delay(step_delay);
            robot_move_left();
            updateScreen();
        }
    }

    void goToOutBox()
    {
        while (robot_column < 6)
        {
            delay(step_delay);
            robot_move_right();
            updateScreen();
        }
    }

    bool handleInbox(bool animate)
    {
        if (in_boxes.size() == 0)
            return false;
        if (animate)
        {
            goToInBox();
        }

        box_taken = in_boxes.front();
        in_boxes.pop_front();

        if (animate)
        {
            delay(step_delay);
            updateScreen();
        }

        return true;
    }

    bool handleOutbox(bool animate)
    {
        if (box_taken.isEmpty)
            return false;
        if (animate)
        {
            goToOutBox();
        }

        out_boxes.push_front(box_taken);
        current_out.push_front(box_taken.data);
        box_taken.empty();

        if (animate)
        {
            delay(step_delay);
            updateScreen();
        }

        return true;
    }

    bool handleCopyto(int x, bool animate)
    {
        if (x < 1)
            return false;
        if (x > playground_boxes.size())
            return false;
        if (box_taken.isEmpty)
            return false;

        if (animate)
        {
            goToPlayGround(x);
        }

        bool isOccupied = !playground_boxes[x - 1].isEmpty;
        playground_boxes[x - 1] = box_taken.data;
        if (isOccupied)
            box_taken.empty();

        if (animate)
        {
            delay(step_delay);
            updateScreen();
        }

        return true;
    }

    bool handleCopyFrom(int x, bool animate)
    {
        if (x < 1)
            return false;
        if (x > playground_boxes.size())
            return false;
        if (playground_boxes[x - 1].isEmpty)
            return false;
        if (animate)
        {
            goToPlayGround(x);
        }

        box_taken = playground_boxes[x - 1];

        if (animate)
        {
            delay(step_delay);
            updateScreen();
        }

        return true;
    }

    bool handleAdd(int x, bool animate)
    {
        if (x < 1)
            return false;
        if (x > playground_boxes.size())
            return false;
        if (playground_boxes[x - 1].isEmpty)
            return false;
        if (box_taken.isEmpty)
            return false;

        if (animate)
        {
            goToPlayGround(x);
        }

        box_taken.data += playground_boxes[x - 1].data;

        if (animate)
        {
            delay(step_delay);
            updateScreen();
        }

        return true;
    }

    bool handleSub(int x, bool animate)
    {
        if (x < 1)
            return false;
        if (x > playground_boxes.size())
            return false;
        if (playground_boxes[x - 1].isEmpty)
            return false;
        if (box_taken.isEmpty)
            return false;

        if (animate)
        {
            goToPlayGround(x);
        }

        box_taken.data -= playground_boxes[x - 1].data;

        if (animate)
        {
            delay(step_delay);
            updateScreen();
        }

        return true;
    }

    bool handleJump(int x)
    {
        if (x < 1)
            return false;
        if (x > codes.size())
            return false;
        current_line = x;
        return true;
    }

    bool handleJumpIfZero(int x, bool &jumped)
    {
        jumped = false;
        if (x < 1)
            return false;
        if (x > codes.size())
            return false;
        if (box_taken.isEmpty)
            return false;

        if (box_taken.data == 0)
        {
            current_line = x;
            jumped = true;
        }
        return true;
    }

public:
    list<Box> in_boxes;
    list<Box> out_boxes;
    vector<Box> playground_boxes;
    vector<Command> codes;
    vector<CommandId> available_command;
    GameScreen screen;
    list<int> expected_out;
    list<int> current_out;
    int current_line;
    int step_delay; // ms

    Box box_taken;
    int robot_column;

    bool runCode()
    {
        current_line = 1;
        bool error = false, done = false;
        while (true)
        {
            Command _command = codes[current_line - 1];
            switch (_command.id)
            {
            case CommandId::inbox:
                done = !handleInbox(true);
                break;
            case CommandId::outbox:
                error = !handleOutbox(true);
                break;
            case CommandId::copyto:
                error = !handleCopyto(_command.arg, true);
                break;
            case CommandId::copyfrom:
                error = !handleCopyFrom(_command.arg, true);
                break;
            case CommandId::add:
                error = !handleAdd(_command.arg, true);
                break;
            case CommandId::sub:
                error = !handleSub(_command.arg, true);
                break;
            case CommandId::jump:
                error = !handleJump(_command.arg);
                if (!error)
                    continue;
            case CommandId::jumpifzero:
                bool jumped;
                error = !handleJumpIfZero(_command.arg, jumped);
                if (jumped)
                    continue;
            }
            current_line++;
            if (current_line > codes.size())
                done = true;

            if (error)
                break;
            if (done)
                break;
        }
        if (error)
        {
            cout << "❌❌❌ Error Occur!";
            return false;
        }
        bool answer_correct = true;
        if (current_out.size() != expected_out.size())
            answer_correct = false;
        else
        {
            auto it1 = current_out.begin();
            auto it2 = expected_out.begin();
            while (it1 != current_out.end() && it2 != expected_out.end())
            {
                if (*it1 != *it2)
                {
                    answer_correct = false;
                    break;
                }
                it1++;
                it2++;
            }
        }
        if (answer_correct)
        {
            cout << "🌟🌟🌟 Mission Accomplished!";
            return true;
        }
        else
        {
            cout << "❌❌❌ Mission Failed!";
            return false;
        }
    }

    Game(vector<int> &in, vector<CommandId> &available_command, int n_playground_boxes, list<int> &expected_out)
    {
        for (int i : in)
            in_boxes.push_back(i);
        playground_boxes.resize(n_playground_boxes);
        this->expected_out = expected_out;
        this->available_command = available_command;
        robot_column = 3;
        current_line = -1;
        step_delay = 500;
    }

    void importCode(string file_path)
    {
        ifstream file(file_path);
        if (!file.is_open())
        {
            cout << "Cannot open file " << file_path << endl;
            return;
        }

        string line;
        int n_command;

        getline(file, line);
        n_command = stoi(line);

        while (n_command--)
        {
            getline(file, line);
            char temp[100];
            int arg = 0;
            int result = sscanf(line.c_str(), "%s %d", temp, &arg);
            if (result == 1)
            {
                addCode(temp);
            }
            else
            {
                addCode(temp, arg);
            }
        }
    }

    bool addCode(string _command)
    {
        CommandId commandId = parseStr(_command);
        if (commandId == CommandId::invalid)
            return false;
        if (find(available_command.begin(), available_command.end(), commandId) != available_command.end())
        {
            codes.push_back(Command(_command));
            return true;
        }
        return false;
    }

    bool addCode(string command, int arg)
    {
        CommandId commandId = parseStr(command);
        if (find(available_command.begin(), available_command.end(), commandId) != available_command.end())
        {
            codes.push_back(Command(command, arg));
            return true;
        }
        return false;
    }

    bool removeCode()
    {
        if (codes.size() == 0)
            return false;
        codes.pop_back();
        return true;
    }

    void updateScreen()
    {
        screen.clear();
        screen.draw(in_boxes, out_boxes, playground_boxes, codes, current_line, robot_column, box_taken);
        // system("cls");
        for (int i = 0; i < 5; i++)
        {
            cleanLineAbove();
        }
        cout << "\033[H";
        screen.print();
    }
};

#include <iostream>
#include <string>

int main()
{
    std::wcout << L"🌟❌🎉 Unicode characters in Windows Terminal!" << std::endl;
    system("pause");
    return 0;
}

/*

int main()
{
    vector<int> in = {1, 2, 3, 4};
    list<int> expected_out = {4, 3, 2, 1};
    vector<CommandId> available_command = {CommandId::inbox, CommandId::outbox};
    Game game(in, available_command, 4, expected_out);
    hideCursor();
    game.updateScreen();
    game.importCode("code.txt");
    // cout << "Enter the command:\n> ";
    // while (true)
    // {
    //     string line;
    //     getline(cin, line);
    //     int arg = 0;
    //     char temp[100];
    //     int result = sscanf(line.c_str(), "%s %d", temp, &arg);

    //     string command(temp);
    //     bool success = false;
    //     if (result == 1)
    //     {
    //         if (command.compare("run") == 0)
    //         {
    //             game.updateScreen();
    //             break;
    //         }
    //         else if (command.compare("delete") == 0)
    //         {
    //             success = game.removeCode();
    //         }
    //         else
    //             success = game.addCode(command);
    //     }
    //     else
    //     {
    //         success = game.addCode(command, arg);
    //     }
    //     game.updateScreen();
    //     if (!success)
    //     {
    //         cout << "Invalid Command!  \n>";
    //         delay(800);
    //         game.updateScreen();
    //     }
    //     cout << "Enter the command:\n> ";
    // }
    game.runCode();
    // game.updateScreen();
    system("pause");
}

 */