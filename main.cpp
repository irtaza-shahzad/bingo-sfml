#include <SFML/Graphics.hpp>
#include <SFML/Audio.hpp>
#include <iostream>
#include <string>
#include <vector>
#include <fstream>
#include <iomanip>
#include <sstream>
#include <algorithm>
#include <cstdlib>
#include <ctime>
using namespace std;

struct OutlinedText : public sf::Text {
public:
    OutlinedText(const string& str, const sf::Font& font, unsigned int charSize) : sf::Text(str, font, charSize) {
        setOutlineThickness(2.0f);
        setOutlineColor(sf::Color::Black);
    }

    void centerOrigin() {
        sf::FloatRect bounds = getLocalBounds();
        setOrigin(bounds.left + bounds.width / 2.0f, bounds.top + bounds.height / 2.0f);
    }
};

struct Button {
public:
    sf::RectangleShape shape;
    OutlinedText text;

    sf::Color normalColor = sf::Color(100, 100, 250);
    sf::Color hoverColor = sf::Color(70, 70, 200);
    sf::Vector2f originalScale = { 1.0f, 1.0f };
    sf::Vector2f hoverScale = { 1.05f, 1.05f };

    Button(const string& t, const sf::Font& font, sf::Vector2f position, sf::Vector2f size) :
        text(t, font, static_cast<unsigned int>(size.y * 0.5f)) {

        shape.setPosition(position);
        shape.setSize(size);
        shape.setOrigin(size.x / 2.0f, size.y / 2.0f);
        shape.setFillColor(normalColor);
        shape.setOutlineColor(sf::Color::White);
        shape.setOutlineThickness(3.0f);

        text.setFillColor(sf::Color::White);
        text.centerOrigin();
        text.setPosition(position);
    }

    void draw(sf::RenderWindow& window) {
        window.draw(shape);
        window.draw(text);
    }

    void update(const sf::Vector2f& mousePos) {
        if (shape.getGlobalBounds().contains(mousePos)) {
            shape.setFillColor(hoverColor);
            shape.setScale(hoverScale);
            text.setScale(hoverScale);
        }
        else {
            shape.setFillColor(normalColor);
            shape.setScale(originalScale);
            text.setScale(originalScale);
        }
    }

    bool isClicked(sf::Event& event, sf::RenderWindow& window) {
        if (event.type == sf::Event::MouseButtonPressed) {
            if (event.mouseButton.button == sf::Mouse::Left) {
                if (shape.getGlobalBounds().contains(window.mapPixelToCoords({ event.mouseButton.x, event.mouseButton.y }))) {
                    return true;
                }
            }
        }
        return false;
    }
};

struct InputBox {
public:
    sf::RectangleShape box;
    OutlinedText text;
    OutlinedText placeholderText;
    string inputString;
    bool isActive = false;
    size_t charLimit = 20;

    InputBox(sf::Vector2f position, sf::Vector2f size, const string& placeholder, const sf::Font& font)
        : text("", font, static_cast<unsigned int>(size.y * 0.6f)),
        placeholderText(placeholder, font, static_cast<unsigned int>(size.y * 0.6f)) {

        box.setSize(size);
        box.setOrigin(size.x / 2.0f, size.y / 2.0f);
        box.setPosition(position);
        box.setFillColor(sf::Color(200, 200, 200));
        box.setOutlineColor(sf::Color::Black);
        box.setOutlineThickness(2.f);

        text.setFillColor(sf::Color::White);
        placeholderText.setFillColor(sf::Color(100, 100, 100));

        this->text.setOutlineThickness(2.0f);
        this->placeholderText.setOutlineThickness(1.0f);

        this->text.setPosition(position.x - size.x / 2.f + 10, position.y - size.y / 2.f);
        this->placeholderText.setPosition(position.x - size.x / 2.f + 10, position.y - size.y / 2.f);
    }

    void handleInput(sf::Event event) {
        if (!isActive) return;
        if (event.type == sf::Event::TextEntered) {
            if (event.text.unicode < 128 && event.text.unicode != 8) {
                if (inputString.size() < charLimit)
                    inputString += static_cast<char>(event.text.unicode);
            }
            else if (event.text.unicode == 8 && !inputString.empty()) {
                inputString.pop_back();
            }
            text.setString(inputString);
        }
    }

    void update(sf::Vector2f mousePos, sf::Event& event) {
        if (event.type == sf::Event::MouseButtonPressed) {
            if (box.getGlobalBounds().contains(mousePos)) {
                isActive = true;
                box.setOutlineColor(sf::Color::Blue);
            }
            else {
                isActive = false;
                box.setOutlineColor(sf::Color::Black);
            }
        }
    }

    void draw(sf::RenderWindow& window) {
        window.draw(box);
        if (inputString.empty() && !isActive) {
            window.draw(placeholderText);
        }
        else {
            window.draw(text);
        }
    }
    string getText() const { return inputString; }
    void setText(const string& str) {
        inputString = str;
        text.setString(str);
    }
};

struct HighScoreEntry {
    string name;
    int score;
};

struct GameHistoryEntry {
    int gameId, size, winnerTurn;
    string player1, player2, winnerName;
};

struct GameSession {
    int player1Board[25][25] = { {0} };
    int player2Board[25][25] = { {0} };
    string p1Name, p2Name;
    int turn = 0;
    int gameId = 0;
    int size = 0;

    void reset() {
        for (int i = 0; i < 25; ++i) {
            for (int j = 0; j < 25; ++j) {
                player1Board[i][j] = 0;
                player2Board[i][j] = 0;
            }
        }
        p1Name = "";
        p2Name = "";
        turn = 0;
        gameId = 0;
        size = 0;
    }
};

enum class GameScreen {
    MAIN_MENU,
    PLAY_MENU,
    NEW_GAME_SETUP,
    HIGH_SCORES,
    GAME_HISTORY,
    IN_GAME,
    EXITING
};

GameSession currentSession;
int& Size = currentSession.size;

bool already(int arr[][25], const int n) {
    for (int i = 0; i < Size; i++) {
        for (int j = 0; j < Size; j++) {
            if (arr[i][j] == n) return true;
        }
    }
    return false;
}

void set_Cards(int arr[][25], const int N) {
    for (int i = 0; i < Size; i++) {
        for (int j = 0; j < Size; j++) {
            int num;
            do {
                num = (rand() % N) + 1;
            } while (already(arr, num));
            arr[i][j] = num;
        }
    }
}

void remove_Num(int arr[][25], const int num) {
    for (int i = 0; i < Size; i++) {
        for (int j = 0; j < Size; j++) {
            if (arr[i][j] == num) {
                arr[i][j] = 0;
            }
        }
    }
}

bool check_Win(int arr[][25]) {
    int rowCount = 0, colCount = 0, diagCount = 0;

    for (int i = 0; i < Size; ++i) {
        bool rowWin = true;
        bool colWin = true;
        for (int j = 0; j < Size; ++j) {
            if (arr[i][j] != 0) rowWin = false;
            if (arr[j][i] != 0) colWin = false;
        }
        if (rowWin) rowCount++;
        if (colWin) colCount++;
    }

    bool mainDiagWin = true;
    bool antiDiagWin = true;
    for (int i = 0; i < Size; ++i) {
        if (arr[i][i] != 0) mainDiagWin = false;
        if (arr[i][Size - 1 - i] != 0) antiDiagWin = false;
    }
    if (mainDiagWin) diagCount++;
    if (antiDiagWin) diagCount++;

    return (rowCount + colCount + diagCount) >= 1;
}

bool validate_Name(const string& name) {
    if (name.empty()) return false;
    for (char c : name) {
        if (!((c >= 'A' && c <= 'Z') || (c >= 'a' && c <= 'z'))) {
            return false;
        }
    }
    return true;
}

void save_Game() {
    ofstream file("SaveGame.txt");
    if (!file) return;

    file << currentSession.size << endl;
    file << currentSession.gameId << endl;
    file << currentSession.turn << endl;
    file << currentSession.p1Name << endl;
    file << currentSession.p2Name << endl;

    for (int i = 0; i < currentSession.size; i++) {
        for (int j = 0; j < currentSession.size; j++) {
            file << currentSession.player1Board[i][j] << " ";
        }
        file << endl;
    }
    for (int i = 0; i < currentSession.size; i++) {
        for (int j = 0; j < currentSession.size; j++) {
            file << currentSession.player2Board[i][j] << " ";
        }
        file << endl;
    }
    file.close();
}

bool resume_Game(string& errorMsg) {
    ifstream file("SaveGame.txt");
    if (!file || file.peek() == EOF || file.peek() == ' ') {
        errorMsg = "No saved game found!";
        return false;
    }

    currentSession.reset();
    file >> currentSession.size;
    file >> currentSession.gameId;
    file >> currentSession.turn;
    file.ignore(numeric_limits<streamsize>::max(), '\n');

    getline(file, currentSession.p1Name);
    getline(file, currentSession.p2Name);

    for (int i = 0; i < currentSession.size; i++) {
        for (int j = 0; j < currentSession.size; j++) {
            file >> currentSession.player1Board[i][j];
        }
    }
    for (int i = 0; i < currentSession.size; i++) {
        for (int j = 0; j < currentSession.size; j++) {
            file >> currentSession.player2Board[i][j];
        }
    }
    file.close();
    return true;
}

void clear_save_file() {
    ofstream file("SaveGame.txt", ios::out | ios::trunc);
    file << " ";
    file.close();
}

vector<HighScoreEntry> read_high_scores() {
    vector<HighScoreEntry> scores;
    ifstream file("HighScore.txt");
    string line;
    while (getline(file, line)) {
        stringstream ss(line);
        string name, scoreStr;
        if (getline(ss, name, ',') && getline(ss, scoreStr)) {
            try {
                int score = stoi(scoreStr);
                if (score != -1) {
                    scores.push_back({ name, score });
                }
            }
            catch (const invalid_argument& e) {}
        }
    }
    sort(scores.begin(), scores.end(), [](const HighScoreEntry& a, const HighScoreEntry& b) {
        return a.score > b.score;
        });
    return scores;
}

void save_record(const string& winnerName) {
    vector<string> names(100);
    vector<int> scores(100, -1);

    ifstream inFile("HighScore.txt");
    int count = 0;
    string line;
    while (count < 100 && getline(inFile, line)) {
        stringstream ss(line);
        string name, scoreStr;
        if (getline(ss, name, ',') && getline(ss, scoreStr)) {
            names[count] = name;
            try { scores[count] = stoi(scoreStr); }
            catch (...) { scores[count] = -1; }
            count++;
        }
    }
    inFile.close();

    bool found = false;
    for (int i = 0; i < 100; ++i) {
        if (names[i] == winnerName) {
            scores[i]++;
            found = true;
            break;
        }
    }
    if (!found) {
        for (int i = 0; i < 100; ++i) {
            if (scores[i] == -1) {
                names[i] = winnerName;
                scores[i] = 1;
                break;
            }
        }
    }

    for (int i = 0; i < 99; i++) {
        for (int j = 0; j < 99 - i; j++) {
            if (scores[j] < scores[j + 1]) {
                swap(scores[j], scores[j + 1]);
                swap(names[j], names[j + 1]);
            }
        }
    }

    ofstream outFile("HighScore.txt");
    for (int i = 0; i < 100; ++i) {
        if (names[i].empty() || names[i] == "NULL" || scores[i] == -1) continue;
        outFile << names[i] << "," << scores[i] << endl;
    }
}


void set_history(int gameId, int size, const string& name1, const string& name2, int turn) {
    vector<string> historyLines;
    ifstream inFile("GameHistory.txt");
    string line;
    while (getline(inFile, line)) {
        historyLines.push_back(line);
    }
    inFile.close();

    while (historyLines.size() >= 50) {
        historyLines.erase(historyLines.begin(), historyLines.begin() + 5);
    }

    ofstream outFile("GameHistory.txt");
    for (const auto& l : historyLines) {
        outFile << l << endl;
    }
    outFile << gameId << endl;
    outFile << size << endl;
    outFile << name1 << endl;
    outFile << name2 << endl;
    outFile << turn << endl;
    outFile.close();
}


vector<GameHistoryEntry> read_game_history() {
    vector<GameHistoryEntry> history;
    ifstream file("GameHistory.txt");
    string idStr, sizeStr, p1, p2, turnStr;

    while (getline(file, idStr) && getline(file, sizeStr) && getline(file, p1) &&
        getline(file, p2) && getline(file, turnStr)) {

        try {
            int turn = stoi(turnStr);
            history.push_back({
                stoi(idStr),
                stoi(sizeStr),
                turn,
                p1,
                p2,
                (turn == 1) ? p1 : p2
                });
        }
        catch (...) {}
    }
    reverse(history.begin(), history.end());
    return history;
}

void show_message_box(sf::RenderWindow& window, const string& title, const string& message, const sf::Font& font, bool& clickConsumedFlag) {
    sf::Vector2f windowSize = sf::Vector2f(window.getSize());
    sf::Vector2f boxSize = { 600, 300 };

    sf::RectangleShape background(windowSize);
    background.setFillColor(sf::Color(0, 0, 0, 150));

    sf::RectangleShape msgBox(boxSize);
    msgBox.setFillColor(sf::Color(220, 220, 220));
    msgBox.setOutlineColor(sf::Color::Black);
    msgBox.setOutlineThickness(3.f);
    msgBox.setOrigin(boxSize / 2.f);
    msgBox.setPosition(windowSize / 2.f);

    OutlinedText titleText(title, font, 32);
    titleText.setFillColor(sf::Color::White);
    titleText.setStyle(sf::Text::Bold);
    titleText.centerOrigin();
    titleText.setPosition(windowSize.x / 2.f, windowSize.y / 2.f - boxSize.y / 2.f + 40);

    OutlinedText messageText(message, font, 24);
    messageText.setFillColor(sf::Color::White);
    messageText.centerOrigin();
    messageText.setPosition(windowSize / 2.f);

    Button okButton("OK", font, { windowSize.x / 2.f, windowSize.y / 2.f + boxSize.y / 2.f - 50.f }, { 150, 60 });

    bool closed = false;
    while (window.isOpen() && !closed) {
        sf::Event event;
        while (window.pollEvent(event)) {
            if (event.type == sf::Event::Closed) {
                window.close();
            }
            if (okButton.isClicked(event, window)) {
                closed = true;
                clickConsumedFlag = true;
            }
        }

        sf::Vector2f mousePos = window.mapPixelToCoords(sf::Mouse::getPosition(window));
        okButton.update(mousePos);

        window.draw(background);
        window.draw(msgBox);
        window.draw(titleText);
        window.draw(messageText);
        okButton.draw(window);
        window.display();
    }
}


int main() {
    srand(static_cast<unsigned>(time(0)));

    sf::VideoMode fullscreenMode = sf::VideoMode::getDesktopMode();
    sf::RenderWindow window(fullscreenMode, "BINGO", sf::Style::Fullscreen);
    window.setFramerateLimit(60);

    sf::Font font;
    if (!font.loadFromFile("assets/arial.ttf")) {
        cerr << "Error: Could not load font from assets/arial.ttf" << endl;
        return EXIT_FAILURE;
    }

    sf::Texture bgTexture;
    if (!bgTexture.loadFromFile("assets/bingo-bg.jpg")) {
        cerr << "Error: Could not load background image 'assets/bingo-bg.jpg'" << endl;
    }
    bgTexture.setSmooth(true);
    sf::Sprite background(bgTexture);
    background.setScale(
        (float)window.getSize().x / bgTexture.getSize().x,
        (float)window.getSize().y / bgTexture.getSize().y
    );

    sf::RectangleShape blurOverlay(sf::Vector2f(window.getSize()));
    blurOverlay.setFillColor(sf::Color(0, 0, 0, 100));

    sf::Music backgroundMusic;
    if (!backgroundMusic.openFromFile("assets/bingo-music.mp3")) {
        cerr << "Warning: Could not load 'assets/bingo-music.mp3'. Continuing without music." << endl;
    }
    else {
        backgroundMusic.setLoop(true);
        backgroundMusic.setVolume(50);
        backgroundMusic.play();
    }

    sf::SoundBuffer clickBuffer;
    if (!clickBuffer.loadFromFile("assets/button-click.mp3")) {
        cerr << "Warning: Could not load 'assets/button-click.mp3'. Continuing without click sounds." << endl;
    }
    sf::Sound clickSound;
    clickSound.setBuffer(clickBuffer);

    GameScreen currentScreen = GameScreen::MAIN_MENU;
    sf::Vector2f windowSize(window.getSize());

    OutlinedText titleText("BINGO", font, 150);
    titleText.setStyle(sf::Text::Bold | sf::Text::Italic);
    titleText.setFillColor(sf::Color::Yellow);
    titleText.centerOrigin();
    titleText.setPosition(windowSize.x / 2.0f, windowSize.y * 0.2f);

    Button playButton("Play Game", font, { windowSize.x / 2, windowSize.y / 2 - 30 }, { 400, 80 });
    Button historyButton("Game History", font, { windowSize.x / 2, windowSize.y / 2 + 80 }, { 400, 80 });
    Button scoresButton("High Scores", font, { windowSize.x / 2, windowSize.y / 2 + 190 }, { 400, 80 });
    Button exitButton("Exit", font, { windowSize.x / 2, windowSize.y / 2 + 300 }, { 400, 80 });

    Button backButton("Back", font, { 100, 70 }, { 150, 60 });

    Button newGameButton("New Game", font, { windowSize.x / 2, windowSize.y / 2 }, { 400, 80 });
    Button resumeGameButton("Resume Game", font, { windowSize.x / 2, windowSize.y / 2 + 110 }, { 400, 80 });

    OutlinedText setupTitle("New Game Setup", font, 60);
    setupTitle.centerOrigin();
    setupTitle.setPosition(windowSize.x / 2, windowSize.y * 0.2f);
    InputBox p1NameInput({ windowSize.x / 2, windowSize.y / 2 - 100 }, { 500, 60 }, "Player 1 Name...", font);
    InputBox p2NameInput({ windowSize.x / 2, windowSize.y / 2 }, { 500, 60 }, "Player 2 Name...", font);
    InputBox sizeInput({ windowSize.x / 2, windowSize.y / 2 + 100 }, { 500, 60 }, "Board Size (4-25)...", font);
    Button startGameButton("Start Game", font, { windowSize.x / 2, windowSize.y / 2 + 220 }, { 300, 80 });
    vector<InputBox*> inputFields = { &p1NameInput, &p2NameInput, &sizeInput };

    vector<HighScoreEntry> highScores;
    vector<GameHistoryEntry> gameHistory;

    Button exitGameButton("Main Menu", font, { windowSize.x - 120, 70 }, { 200, 60 });
    vector<vector<Button>> p1CardButtons;
    vector<vector<Button>> p2CardButtons;
    bool needsBoardRedraw = true;


    while (window.isOpen()) {
        bool mouseClickConsumed = false;

        sf::Event event;
        while (window.pollEvent(event)) {
            if (event.type == sf::Event::Closed) {
                currentScreen = GameScreen::EXITING;
            }

            if (currentScreen == GameScreen::NEW_GAME_SETUP) {
                sf::Vector2f mousePos = window.mapPixelToCoords(sf::Mouse::getPosition(window));
                for (auto& input : inputFields) {
                    input->update(mousePos, event);
                    input->handleInput(event);
                }
            }

            if (event.type == sf::Event::MouseButtonPressed) {

                switch (currentScreen) {
                case GameScreen::MAIN_MENU:
                    if (playButton.isClicked(event, window)) { currentScreen = GameScreen::PLAY_MENU; mouseClickConsumed = true; }
                    else if (historyButton.isClicked(event, window)) { gameHistory = read_game_history(); currentScreen = GameScreen::GAME_HISTORY; mouseClickConsumed = true; }
                    else if (scoresButton.isClicked(event, window)) { highScores = read_high_scores(); currentScreen = GameScreen::HIGH_SCORES; mouseClickConsumed = true; }
                    else if (exitButton.isClicked(event, window)) { currentScreen = GameScreen::EXITING; mouseClickConsumed = true; }
                    break;
                case GameScreen::PLAY_MENU:
                    if (newGameButton.isClicked(event, window)) { currentScreen = GameScreen::NEW_GAME_SETUP; mouseClickConsumed = true; }
                    else if (resumeGameButton.isClicked(event, window)) {
                        string errorMsg;
                        if (resume_Game(errorMsg)) {
                            needsBoardRedraw = true;
                            currentScreen = GameScreen::IN_GAME;
                        }
                        else {
                            show_message_box(window, "Error", errorMsg, font, mouseClickConsumed);
                        }
                        mouseClickConsumed = true;
                    }
                    else if (backButton.isClicked(event, window)) { currentScreen = GameScreen::MAIN_MENU; mouseClickConsumed = true; }
                    break;
                case GameScreen::NEW_GAME_SETUP:
                    if (backButton.isClicked(event, window)) { currentScreen = GameScreen::PLAY_MENU; mouseClickConsumed = true; }
                    else if (startGameButton.isClicked(event, window)) {
                        string p1 = p1NameInput.getText();
                        string p2 = p2NameInput.getText();
                        string sizeStr = sizeInput.getText();
                        string error;

                        if (!validate_Name(p1)) error = "Player 1 name is invalid.\n(Numbers/Special characters are not allowed)";
                        else if (!validate_Name(p2)) error = "Player 2 name is invalid.\n(Numbers/Special characters are not allowed)";
                        else if (p1 == p2) error = "Player names cannot be the same.";
                        else {
                            try {
                                int s = stoi(sizeStr);
                                if (s < 4 || s > 25) error = "Size must be between 4 and 25.";
                            }
                            catch (...) { error = "Invalid size entered."; }
                        }
                        if (!error.empty()) {
                            show_message_box(window, "Validation Error", error, font, mouseClickConsumed);
                        }
                        else {
                            currentSession.reset();
                            currentSession.p1Name = p1;
                            currentSession.p2Name = p2;
                            currentSession.size = stoi(sizeStr);
                            int numbers = Size * Size;

                            set_Cards(currentSession.player1Board, numbers);
                            set_Cards(currentSession.player2Board, numbers);
                            currentSession.gameId = (rand() % 9000) + 1000;
                            currentSession.turn = (rand() % 2) + 1;

                            save_Game();
                            needsBoardRedraw = true;
                            currentScreen = GameScreen::IN_GAME;
                        }
                        mouseClickConsumed = true;
                    }
                    break;
                case GameScreen::HIGH_SCORES:
                case GameScreen::GAME_HISTORY:
                    if (backButton.isClicked(event, window)) { currentScreen = GameScreen::MAIN_MENU; mouseClickConsumed = true; }
                    break;
                case GameScreen::IN_GAME:
                    if (exitGameButton.isClicked(event, window)) { save_Game(); currentScreen = GameScreen::MAIN_MENU; mouseClickConsumed = true; }
                    else {
                        bool numberClicked = false;
                        int clickedNumber = -1;

                        vector<vector<Button>>* activeBoard = (currentSession.turn == 1) ? &p1CardButtons : &p2CardButtons;

                        for (int i = 0; i < Size; ++i) {
                            for (int j = 0; j < Size; ++j) {
                                if ((*activeBoard)[i][j].isClicked(event, window)) {
                                    if ((currentSession.turn == 1 && currentSession.player1Board[i][j] != 0) ||
                                        (currentSession.turn == 2 && currentSession.player2Board[i][j] != 0)) {
                                        clickedNumber = stoi((*activeBoard)[i][j].text.getString().toAnsiString());
                                        numberClicked = true;
                                        break;
                                    }
                                }
                            }
                            if (numberClicked) break;
                        }

                        if (numberClicked) {
                            remove_Num(currentSession.player1Board, clickedNumber);
                            remove_Num(currentSession.player2Board, clickedNumber);
                            bool hasWon = (currentSession.turn == 1) ? check_Win(currentSession.player1Board) : check_Win(currentSession.player2Board);
                            if (hasWon) {
                                string winner = (currentSession.turn == 1) ? currentSession.p1Name : currentSession.p2Name;
                                show_message_box(window, "Game Over!", winner + " wins!", font, mouseClickConsumed);
                                save_record(winner);
                                set_history(currentSession.gameId, Size, currentSession.p1Name, currentSession.p2Name, currentSession.turn);
                                clear_save_file();
                                currentScreen = GameScreen::MAIN_MENU;
                            }
                            else {
                                currentSession.turn = (currentSession.turn == 1) ? 2 : 1;
                                save_Game();
                            }
                            needsBoardRedraw = true;
                            mouseClickConsumed = true;
                        }
                    }
                    break;
                default: break;
                }

                if (mouseClickConsumed) {
                    clickSound.play();
                }
            }
        }

        sf::Vector2f mousePos = window.mapPixelToCoords(sf::Mouse::getPosition(window));

        //window.clear(sf::Color(20, 20, 80));

        window.clear();
        window.draw(background);
        window.draw(blurOverlay);

        switch (currentScreen) {
        case GameScreen::MAIN_MENU:
            playButton.update(mousePos); historyButton.update(mousePos); scoresButton.update(mousePos); exitButton.update(mousePos);
            window.draw(titleText); playButton.draw(window); historyButton.draw(window); scoresButton.draw(window); exitButton.draw(window);
            break;
        case GameScreen::PLAY_MENU:
            newGameButton.update(mousePos); resumeGameButton.update(mousePos); backButton.update(mousePos);
            newGameButton.draw(window); resumeGameButton.draw(window); backButton.draw(window);
            break;
        case GameScreen::NEW_GAME_SETUP:
            backButton.update(mousePos); startGameButton.update(mousePos);
            window.draw(setupTitle);
            for (auto& input : inputFields) input->draw(window);
            startGameButton.draw(window); backButton.draw(window);
            break;
        case GameScreen::HIGH_SCORES: {
            backButton.update(mousePos);
            OutlinedText hsTitle("High Scores", font, 60); hsTitle.centerOrigin(); hsTitle.setPosition(windowSize.x / 2, 100);
            window.draw(hsTitle);
            float startY = 200.f; int rank = 1;
            for (const auto& entry : highScores) {
                if (rank > 10) break;
                OutlinedText rankText(to_string(rank) + ".", font, 30); OutlinedText nameText(entry.name, font, 30); OutlinedText scoreText(to_string(entry.score), font, 30);
                rankText.setPosition(windowSize.x / 2 - 250, startY + (rank * 50)); nameText.setPosition(windowSize.x / 2 - 150, startY + (rank * 50)); scoreText.setPosition(windowSize.x / 2 + 200, startY + (rank * 50));
                window.draw(rankText); window.draw(nameText); window.draw(scoreText); rank++;
            }
            backButton.draw(window);
            break;
        }
        case GameScreen::GAME_HISTORY: {
            backButton.update(mousePos);
            OutlinedText ghTitle("Game History", font, 60); ghTitle.centerOrigin(); ghTitle.setPosition(windowSize.x / 2, 80);
            window.draw(ghTitle);
            float startY = 180.f; int count = 0;
            for (const auto& entry : gameHistory) {
                if (count >= 10) break;
                string line1 = "Game ID: " + to_string(entry.gameId) + " | Size: " + to_string(entry.size); string line2 = "Players: " + entry.player1 + " vs " + entry.player2; string line3 = "Winner: " + entry.winnerName;
                sf::RectangleShape entryBg({ 900, 100 }); entryBg.setFillColor(sf::Color(255, 255, 255, 30)); entryBg.setOrigin(450, 50); entryBg.setPosition(windowSize.x / 2, startY + (count * 120));
                OutlinedText text1(line1, font, 24); text1.setPosition(entryBg.getPosition().x - 430, entryBg.getPosition().y - 40); OutlinedText text2(line2, font, 24); text2.setPosition(entryBg.getPosition().x - 430, entryBg.getPosition().y - 10); OutlinedText text3(line3, font, 24); text3.setPosition(entryBg.getPosition().x - 430, entryBg.getPosition().y + 20);
                window.draw(entryBg); window.draw(text1); window.draw(text2); window.draw(text3); count++;
            }
            backButton.draw(window);
            break;
        }
        case GameScreen::IN_GAME: {
            if (needsBoardRedraw) {
                p1CardButtons.clear(); p2CardButtons.clear(); float totalBoardWidth = (windowSize.x - 200) / 2.f - 100; float cellSize = min(60.f, totalBoardWidth / Size); sf::Vector2f p1BoardStart = { 100, 200 }; sf::Vector2f p2BoardStart = { windowSize.x / 2.f + 50, 200 };
                p1CardButtons.resize(Size); p2CardButtons.resize(Size);
                for (int i = 0; i < Size; ++i) {
                    p1CardButtons[i].clear(); p2CardButtons[i].clear();
                    for (int j = 0; j < Size; ++j) {
                        p1CardButtons[i].push_back(Button(to_string(currentSession.player1Board[i][j]), font, sf::Vector2f(p1BoardStart.x + j * cellSize + cellSize / 2, p1BoardStart.y + i * cellSize + cellSize / 2), sf::Vector2f(cellSize - 2, cellSize - 2)));
                        p2CardButtons[i].push_back(Button(to_string(currentSession.player2Board[i][j]), font, sf::Vector2f(p2BoardStart.x + j * cellSize + cellSize / 2, p2BoardStart.y + i * cellSize + cellSize / 2), sf::Vector2f(cellSize - 2, cellSize - 2)));
                    }
                }
                needsBoardRedraw = false;
            }
            exitGameButton.update(mousePos);
            OutlinedText p1NameText(currentSession.p1Name, font, 40); p1NameText.setPosition(100, 150); OutlinedText p2NameText(currentSession.p2Name, font, 40); p2NameText.setPosition(windowSize.x / 2.f + 50, 150);
            string turnString = "Turn: " + (currentSession.turn == 1 ? currentSession.p1Name : currentSession.p2Name); OutlinedText turnText(turnString, font, 50); turnText.centerOrigin(); turnText.setPosition(windowSize.x / 2.f, 80);
            window.draw(p1NameText); window.draw(p2NameText); window.draw(turnText);

            for (int i = 0; i < Size; ++i) {
                for (int j = 0; j < Size; ++j) {
                    Button& btn = p1CardButtons[i][j]; bool isCrossedOut = currentSession.player1Board[i][j] == 0;
                    if (isCrossedOut) { btn.text.setString("X"); btn.text.setFillColor(sf::Color::Red); btn.shape.setFillColor(sf::Color(150, 50, 50)); }
                    btn.update(mousePos); btn.draw(window);
                }
            }
            for (int i = 0; i < Size; ++i) {
                for (int j = 0; j < Size; ++j) {
                    Button& btn = p2CardButtons[i][j]; bool isCrossedOut = currentSession.player2Board[i][j] == 0;
                    if (isCrossedOut) { btn.text.setString("X"); btn.text.setFillColor(sf::Color::Red); btn.shape.setFillColor(sf::Color(150, 50, 50)); }
                    btn.update(mousePos); btn.draw(window);
                }
            }
            exitGameButton.draw(window);
            break;
        }
        case GameScreen::EXITING:
            window.close();
            break;
        }

        window.display();
    }

    return EXIT_SUCCESS;
}