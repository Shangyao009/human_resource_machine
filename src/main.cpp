#include <iostream>
#include <vector>

using namespace std;

const string title = "Human Resource Machine";

void clearScreen()
{
    system("cls");
}

void renderTitle()
{
    cout << R"( _   _                               ____                                    )" << endl;
    cout << R"(| | | |_   _ _ __ ___   __ _ _ __   |  _ \ ___  ___  ___  _   _ _ __ ___ ___ )" << endl;
    cout << R"(| |_| | | | | '_ ` _ \ / _` | '_ \  | |_) / _ \/ __|/ _ \| | | | '__/ __/ _ \)" << endl;
    cout << R"(|  _  | |_| | | | | | | (_| | | | | |  _ <  __/\__ \ (_) | |_| | | | (_|  __/)" << endl;
    cout << R"(|_| |_|\__,_|_| |_|_|_|\__,_|_| |_| |_| \_\___||___/\___/ \__,_|_|  \___\___|)" << endl;
    cout << R"( _   __            _     _                                                   )" << endl;
    cout << R"(|  \/  | __ _  ___| |__ (_)_ __   ___                                        )" << endl;
    cout << R"(| |\/| |/ _` |/ __| '_ \| | '_ \ / _ \                                       )" << endl;
    cout << R"(| |  | | (_| | (__| | | | | | | |  __/                                       )" << endl;
    cout << R"(|_|  |_|\__,_|\___|_| |_|_|_| |_|\___|                                       )" << endl;
    cout << endl;

    cout << "---------------------------------------------------------------------------------" << endl;

    cout << endl;
}

void renderMenu()
{
    clearScreen();
    renderTitle();
    cout << "1. Level 1" << endl;
    cout << "2. Level 2" << endl;
    cout << "3. Level 3" << endl;
    cout << "4. Level 4" << endl;
    cout << "5. Level 5" << endl;

    cout << endl;
    cout << "Select The level: ";
    int i = 0;
    cin >> i;
    cout << "Your have selected level " << i << endl;
}

int main()
{
    // renderMenu();

    system("pause");
    return 0;
}