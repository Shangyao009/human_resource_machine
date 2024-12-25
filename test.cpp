#include <iostream>
#include <vector>
#include <list>
#include <string>
#include <chrono>
#include <thread>
#include <algorithm>
#include <fstream>

using namespace std;

enum class CommandId : int
{
    inbox = 0,
    outbox = 1,
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

class GameInfo
{
public:
    string title;
    vector<int> in;
    list<int> expected_out;
    vector<CommandId> available_command;
    int n_playground;
    GameInfo() : title(""), in({}), expected_out({}), available_command({}), n_playground(0) {}
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

enum class Result
{
    idle,
    success,
    failed,
    error
};

class GameScreen
{
    const int SCREEN_LEN = 100;
    const int SCREEN_HEIGHT = 20;
    const int BOX_HEIGHT = 3;
    const int BOX_WIDTH = 5;
    const int SPLIT_SCREEN_COLUMN = 63;
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
            screen[sr + BOX_HEIGHT][c + (BOX_WIDTH - 1) / 2] = to_string(i + 1)[0];
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

    void drawCodeBlock(vector<string> &code, int current_line = -1, int MAX_LINE = 5)
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
            string command = code[i];
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
    void drawResultBlock(Result status, int step_used = 0)
    {
        int _r = 10;
        for (int i = 0; i < 5; i++)
        {
            screen[_r][SPLIT_SCREEN_COLUMN + 2 + i] = '=';
        }
        screen[_r][SPLIT_SCREEN_COLUMN + 8] = 'S';
        screen[_r][SPLIT_SCREEN_COLUMN + 9] = 'T';
        screen[_r][SPLIT_SCREEN_COLUMN + 10] = 'A';
        screen[_r][SPLIT_SCREEN_COLUMN + 11] = 'T';
        screen[_r][SPLIT_SCREEN_COLUMN + 12] = 'U';
        screen[_r][SPLIT_SCREEN_COLUMN + 13] = 'S';

        for (int i = 1; i < 5; i++)
        {
            screen[_r][SPLIT_SCREEN_COLUMN + 14 + i] = '=';
        }

        string text = "> idle";
        if (status == Result::error)
            text = "> Error while Running!";
        else if (status == Result::success)
            text = "> Mission Accomplished!";
        else if (status == Result::failed)
            text = "> Mission Failed!";
        int _c = SPLIT_SCREEN_COLUMN + 4;
        _r += 1;
        for (int i = 0; i < text.length(); i++)
            screen[_r][_c + i] = text[i];

        if (step_used <= 0)
            return;
        text = "> Steps Used: " + to_string(step_used);
        _r += 1;
        for (int i = 0; i < text.length(); i++)
            screen[_r][_c + i] = text[i];
    }

    void drawAvailableCommand(vector<CommandId> &available_command)
    {
        int _r = 10;
        for (int i = 0; i < 5; i++)
        {
            screen[_r][SPLIT_SCREEN_COLUMN + 2 + i] = '=';
        }
        screen[_r][SPLIT_SCREEN_COLUMN + 8] = 'V';
        screen[_r][SPLIT_SCREEN_COLUMN + 9] = 'A';
        screen[_r][SPLIT_SCREEN_COLUMN + 10] = 'L';
        screen[_r][SPLIT_SCREEN_COLUMN + 11] = 'I';
        screen[_r][SPLIT_SCREEN_COLUMN + 12] = 'D';

        for (int i = 1; i < 5; i++)
        {
            screen[_r][SPLIT_SCREEN_COLUMN + 13 + i] = '=';
        }

        for (int i = 0; i < available_command.size(); i++)
        {
            string command = toStr(available_command[i]);
            if (available_command[i] >= CommandId::add)
                command += " x";
            int _c = SPLIT_SCREEN_COLUMN + 4;
            _r += 1;
            screen[_r][_c] = to_string(i + 1)[0];
            for (int j = 0; j < command.length(); j++)
                screen[_r][_c + 2 + j] = command[j];
        }
    }

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

    void draw(list<Box> &in, list<Box> &out, vector<Box> &playground, vector<string> &code, int current_line, int robot_column, Box &box_taken)
    {
        // draw In Boxes
        drawBoxesVertical(BOX_WIDTH + 1, in, 6);
        // draw out boxes
        drawBoxesVertical(8 * (BOX_WIDTH + 1), out, 6);

        drawPlaygroundBoxes(3.5f * BOX_HEIGHT, 3 * (BOX_WIDTH + 1), playground, playground.size());
        drawRobot(BOX_HEIGHT, robot_column * (BOX_WIDTH + 1), box_taken);

        drawSeperateLine();
        drawCodeBlock(code, current_line, 8);
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

void delay(int ms);
void cleanLineAbove();

class Game
{
    void goToPlayGround(int x)
    {
        int target_column = 3 + x;
        while (robot_column < target_column)
        {
            delay(step_delay);
            robot_column += 1;
            updateScreen();
        }
        while (robot_column > target_column)
        {
            delay(step_delay);
            robot_column -= 1;
            updateScreen();
        }
    }

    void goToInBox()
    {
        while (robot_column > 3)
        {
            delay(step_delay);
            robot_column -= 1;
            updateScreen();
        }
    }

    void goToOutBox()
    {
        while (robot_column < 6)
        {
            delay(step_delay);
            robot_column += 1;
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
        current_out.push_back(box_taken.data);
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
        if (x < 0)
            return false;
        if (x >= playground_boxes.size())
            return false;
        if (box_taken.isEmpty)
            return false;

        if (animate)
        {
            goToPlayGround(x);
        }

        bool isOccupied = !playground_boxes[x].isEmpty;
        playground_boxes[x] = box_taken.data;
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
        if (x < 0)
            return false;
        if (x >= playground_boxes.size())
            return false;
        if (playground_boxes[x].isEmpty)
            return false;
        if (animate)
        {
            goToPlayGround(x);
        }

        box_taken = playground_boxes[x];

        if (animate)
        {
            delay(step_delay);
            updateScreen();
        }

        return true;
    }

    bool handleAdd(int x, bool animate)
    {
        if (x < 0)
            return false;
        if (x >= playground_boxes.size())
            return false;
        if (playground_boxes[x].isEmpty)
            return false;
        if (box_taken.isEmpty)
            return false;

        if (animate)
        {
            goToPlayGround(x);
        }

        box_taken.data += playground_boxes[x].data;

        if (animate)
        {
            delay(step_delay);
            updateScreen();
        }

        return true;
    }

    bool handleSub(int x, bool animate)
    {
        if (x < 0)
            return false;
        if (x >= playground_boxes.size())
            return false;
        if (playground_boxes[x].isEmpty)
            return false;
        if (box_taken.isEmpty)
            return false;

        if (animate)
        {
            goToPlayGround(x);
        }

        box_taken.data -= playground_boxes[x].data;

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

    bool isValidCommand(const string &line, pair<CommandId, int> &command)
    {
        char t1[100];
        char t2[100];
        char t3[100];

        int c = sscanf(line.c_str(), "%s %s %s", t1, t2, t3);
        if (c <= 0 || c > 2)
            return false;
        string cmd(t1);
        CommandId id = parseStr(cmd);
        if (id == CommandId::invalid)
            return false;
        if (id < CommandId::add && c > 1)
            return false;
        if (id >= CommandId::add && c < 2)
            return false;
        if (find(available_command.begin(), available_command.end(), id) == available_command.end())
            return false;
        if (c == 1) // no arg command
        {
            command = {id, 0};
            return true;
        }
        int arg;
        c = sscanf(t2, "%d", &arg);
        if (c == 0)
            return false;
        command = {id, arg};
        return true;
    }

public:
    list<Box> in_boxes;
    list<Box> ori_in;
    list<Box> out_boxes;
    vector<Box> playground_boxes;
    vector<string> codes;
    vector<CommandId> available_command;
    GameScreen screen;
    list<int> expected_out;
    list<int> current_out;
    int current_line;
    int step_delay; // ms
    bool logAvailableCommand;
    Result prevResult;
    int step_used;
    string title;

    Box box_taken;
    int robot_column;

    Game(string title, vector<int> &in, vector<CommandId> &available_command, int n_playground_boxes, list<int> &expected_out)
    {
        this->title = title;
        for (int i : in)
        {
            in_boxes.push_back(i);
            ori_in.push_back(i);
        }
        playground_boxes.resize(n_playground_boxes);
        this->expected_out = expected_out;
        this->available_command = available_command;
        robot_column = 3;
        current_line = -1;
        step_delay = 500;
        logAvailableCommand = true;
        prevResult = Result::idle;
        step_used = 0;
    }
    void updateScreen()
    {
        screen.clear();
        screen.draw(in_boxes, out_boxes, playground_boxes, codes, current_line, robot_column, box_taken);
        if (logAvailableCommand)
            screen.drawAvailableCommand(available_command);
        else
            screen.drawResultBlock(prevResult, step_used);
        // system("cls");
        for (int i = 0; i < 5; i++)
        {
            cleanLineAbove();
        }
        cout << "\033[H";
        screen.print();
        // print ori in
        cout << "Ori In: ";
        for (Box &box : ori_in)
        {
            cout << box.data << " ";
        }
        cout << endl;
        // print target output
        cout << "Expected Out: ";
        for (int i : expected_out)
        {
            cout << i << " ";
        }
        cout << endl;
        cout << endl;
    }

    bool runCode(bool animate)
    {
        step_used = 0;
        prevResult = Result::idle;
        logAvailableCommand = false;
        current_line = 1;
        out_boxes.clear();
        current_out.clear();
        for (Box &box : playground_boxes)
            box.empty();
        box_taken.empty();
        in_boxes = ori_in;
        if (codes.size() == 0)
        {
            prevResult = Result::error;
            return false;
        }
        bool error = false, done = false;
        while (true)
        {
            step_used++;
            string code = codes[current_line - 1];
            pair<CommandId, int> command;
            error = !isValidCommand(code, command);
            if (error)
                break;
            int arg = command.second;
            switch (command.first)
            {
            case CommandId::inbox:
                done = !handleInbox(animate);
                break;
            case CommandId::outbox:
                error = !handleOutbox(animate);
                break;
            case CommandId::copyto:
                error = !handleCopyto(arg, animate);
                break;
            case CommandId::copyfrom:
                error = !handleCopyFrom(arg, animate);
                break;
            case CommandId::add:
                error = !handleAdd(arg, animate);
                break;
            case CommandId::sub:
                error = !handleSub(arg, animate);
                break;
            case CommandId::jump:
                error = !handleJump(arg);
                if (!error)
                    continue;
                break;
            case CommandId::jumpifzero:
                bool jumped;
                error = !handleJumpIfZero(arg, jumped);
                if (jumped)
                    continue;
                break;
            }
            if (error)
                break;
            current_line++;
            if (current_line > codes.size())
                done = true;
            if (done)
                break;
        }
        if (error)
        {
            prevResult = Result::error;
            return false;
        }
        current_line = -1;
        if (resultMatched())
        {
            prevResult = Result::success;
            return true;
        }
        else
        {
            prevResult = Result::failed;
            return false;
        }
    }

    bool resultMatched()
    {
        if (current_out.size() != expected_out.size())
            return false;
        else
        {
            auto it1 = current_out.begin();
            auto it2 = expected_out.begin();
            while (it1 != current_out.end() && it2 != expected_out.end())
            {
                if (*it1 != *it2)
                    return false;
                it1++;
                it2++;
            }
        }
        return true;
    }

    bool importCode(string file_path)
    {
        ifstream file(file_path);
        if (!file.is_open())
        {
            return false;
        }
        string line;
        int n_command;
        try
        {
            getline(file, line);
            n_command = stoi(line);
        }
        catch (...)
        {
            return false;
        }
        codes.clear();
        while (n_command--)
        {
            getline(file, line);
            addCode(line);
        }
        return true;
    }

    void addCode(string code)
    {
        codes.push_back(code);
    }

    void removeCode()
    {
        codes.pop_back();
    }
};

void playGame(string title, vector<int> &in, vector<CommandId> &available_command, int n_playground, list<int> &expected_out, string &fname)
{
    Game game(title, in, available_command, n_playground, expected_out);
    if (fname.size() > 0)
        game.importCode(fname);
    game.updateScreen();

    while (true)
    {
        game.updateScreen();
        string line;
        cout << "Enter the command: ( run / exit / add / import ) \n> ";
        getline(cin, line);
        if (line.compare("run") == 0)
            game.runCode(true);
        else if (line.compare("exit") == 0)
            break;
        else if (line.compare("add") == 0)
        {
            game.logAvailableCommand = true;
            while (true)
            {
                game.updateScreen();
                cout << "Add Code Mode: (input \'q\' if done, \'d\' for delete, \'c\' for clear)\n> ";
                char temp[100];
                int arg = 0;
                getline(cin, line);
                if (line.compare("q") == 0)
                {
                    game.logAvailableCommand = false;
                    break;
                }
                if (line.compare("d") == 0)
                {
                    game.removeCode();
                    continue;
                }
                if (line.compare("c") == 0)
                {
                    game.codes.clear();
                    continue;
                }
                game.addCode(line);
            }
        }
        else if (line.compare("import") == 0)
        {
            game.updateScreen();
            cout << "Enter the file path: (q to quit)\n> ";
            string file_path;
            getline(cin, file_path);
            if (file_path.compare("q") == 0)
                continue;
            bool success = game.importCode(file_path);
            if (!success)
            {
                cout << "Cannot import code from file " << file_path << endl;
                delay(800);
            }
        }
    }
}

void playGame(GameInfo &info, string &importFile)
{
    return playGame(info.title, info.in, info.available_command, info.n_playground, info.expected_out, importFile);
}

GameInfo gameInfo1;
GameInfo gameInfo2;
GameInfo gameInfo3;
GameInfo gameInfo4;

void initGameInfo()
{
    gameInfo1.title = "Level 1";
    gameInfo1.in = {1, 2};
    gameInfo1.expected_out = {1, 2};
    gameInfo1.available_command = {CommandId::inbox, CommandId::outbox};
    gameInfo1.n_playground = 0;

    gameInfo2.title = "Level 2";
    gameInfo2.in = {3, 9, 5, 1, -2, -2, 9, -9};
    gameInfo2.expected_out = {-6, 6, 4, -4, 0, 0, 18, -18};
    gameInfo2.n_playground = 3;
    gameInfo2.available_command = {CommandId::inbox, CommandId::outbox, CommandId::add, CommandId::sub, CommandId::copyto, CommandId::copyfrom, CommandId::jump, CommandId::jumpifzero};

    gameInfo3.title = "Level 3";
    gameInfo3.in = {6, 2, 7, 7, -9, 3, -3, -3};
    gameInfo3.expected_out = {7, -3};
    gameInfo3.n_playground = 3;
    gameInfo3.available_command = {CommandId::inbox, CommandId::outbox, CommandId::add, CommandId::sub, CommandId::copyto, CommandId::copyfrom, CommandId::jump, CommandId::jumpifzero};

    gameInfo4.title = "Level 4";
    gameInfo4.in = {1};
    gameInfo4.expected_out = {1, 1, 2, 3};
    gameInfo4.n_playground = 4;
    gameInfo4.available_command = {CommandId::inbox, CommandId::outbox, CommandId::add, CommandId::sub, CommandId::copyto, CommandId::copyfrom, CommandId::jump, CommandId::jumpifzero};
}

void simulate(GameInfo &info)
{
    Game game(info.title, info.in, info.available_command, info.n_playground, info.expected_out);
    string line;
    int n_op;
    getline(cin, line);
    sscanf(line.c_str(), "%d", &n_op);
    for (int i = 1; i <= n_op; i++)
    {
        getline(cin, line);
        game.addCode(line);
    }
    game.runCode(false);
    if (game.prevResult == Result::error)
        cout << "Error on instruction " << game.current_line << endl;
    else if (game.prevResult == Result::failed)
        cout << "Fail" << endl;
    else
        cout << "Success" << endl;
}

void cleanLineAbove()
{
    // Move up one line
    cout << "\033[A";
    // Clear the line
    cout << "\033[K";
}

#define ojTest

#ifdef ojTest
void delay(int ms) {}
#else
#include <windows.h>
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

#endif

int main(int argc, char *argv[])
{
    initGameInfo();
    int _d = -1;
    if (argc > 1)
        _d = stoi(argv[1]);
#ifdef ojTest

    ifstream in("in.txt");
    ofstream out("out.txt");
    cin.rdbuf(in.rdbuf());
    cout.rdbuf(out.rdbuf());
    ofstream debug("debug.txt");

    string line;
    int n_epoch;
    getline(cin, line);
    sscanf(line.c_str(), "%d", &n_epoch);

    for (int i = 0; i < n_epoch; i++)
    {
        if (i == (_d - 1))
        {
            getline(cin, line);
            debug << line << endl;
            getline(cin, line);
            debug << line << endl;
            int n_op = 0;
            sscanf(line.c_str(), "%d", &n_op);
            for (int i = 0; i < n_op; i++)
            {
                getline(cin, line);
                debug << line << endl;
            }
            return 0;
        }
        int level;
        getline(cin, line);
        sscanf(line.c_str(), "%d", &level);
        GameInfo *game;
        switch (level)
        {
        case 1:
            game = &gameInfo1;
            break;
        case 2:
            game = &gameInfo2;
            break;
        case 3:
            game = &gameInfo3;
            break;
        default:
            return 0;
            // game = &gameInfo4;
            // break;
        }
        simulate(*game);
    }
    // system("pause");

#else

    string f;
    hideCursor();
    playGame(gameInfo1, f);

#endif
    return 0;
}
