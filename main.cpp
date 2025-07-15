#include <SFML/Graphics.hpp>
#include <vector>
#include <thread>
#include <iostream>
#include <cmath>
#include <functional>
#include <fstream>
#include <mutex>
#include "gnuplot.h"
using namespace std;

int seed = time(NULL);
int longitud_cadena;
int numRows;
int neighbours = 1;
int regla = 30;
int probability = 50;
int ones;
float prev_mean = 0;
sf::Color alive = sf::Color::White;
sf::Color dead = sf::Color::Black;
mutex mtx;

vector<bool> reglas;
vector<int> veces_regla;

class Button {
public:
    sf::Color color = sf::Color::Blue;
    sf::Color mouseover = sf::Color::Red;
    sf::Color border_color = sf::Color::Black;
    sf::RectangleShape border;

    Button(sf::RenderWindow& window, const std::string& text, const sf::Vector2f& position, const sf::Vector2f& size)
            : window(window), text(text), position(position), size(size), isClicked(false) {
        rect.setSize(size);
        rect.setPosition(position);
        rect.setFillColor(color);

        this->position.setSize(size);
        this->position.setPosition(position);

        sf::Vector2f temp(size.x + 2, size.y + 2);
        border.setSize(temp);
        temp = {position.x - 1, position.y - 1};
        border.setPosition(temp);
        border.setFillColor(border_color);

        font.loadFromFile("C:\\fonts\\roboto.ttf");
        buttonText.setFont(font);
        buttonText.setString(text);
        buttonText.setCharacterSize(20);
        buttonText.setPosition(position.x + 10, position.y + 10);

        onClick = []() {};
    }

    void draw() {
        border.setFillColor(border_color);
        window.draw(border);
        window.draw(rect);
        window.draw(buttonText);
    }

    void update() {
        if (isMouseOver()) {
            rect.setFillColor(mouseover);
            if (sf::Mouse::isButtonPressed(sf::Mouse::Left)) {
                isClicked = true;
                onClick();
            }
        } else {
            rect.setFillColor(color);
            isClicked = false;
        }
    }

    bool wasClicked() const {
        return isClicked;
    }

    void setOnClick(const std::function<void()>& action) {
        onClick = action;
    }

    void setPosition(int x, int y){
        rect.setPosition(x, y);
        border.setPosition(x-1, y-1);
        buttonText.setPosition(x + 10, y + 10);
    }

private:
    sf::RenderWindow& window;
    sf::RectangleShape rect;
    sf::Font font;
    sf::Text buttonText;
    std::string text;
    sf::RectangleShape position;
    sf::Vector2f size;
    bool isClicked;
    std::function<void()> onClick;

    bool isMouseOver() const {
        sf::Vector2i mousePos = sf::Mouse::getPosition(window);
        return position.getGlobalBounds().contains(static_cast<float>(mousePos.x), static_cast<float>(mousePos.y));
    }
};

class TextInputBox {
public:
    TextInputBox(sf::RenderWindow& window, sf::Font& font, int x, int y, int width, int height, string s)
            : window(window), font(font), isFocused(false) {
        inputBox.setSize(sf::Vector2f(width, height));
        inputBox.setPosition(x, y);
        inputBox.setOutlineThickness(2);
        inputBox.setOutlineColor(sf::Color::Black);
        inputBox.setFillColor(sf::Color::White);

        this->position.setSize(sf::Vector2f(width, height));
        this->position.setPosition(sf::Vector2f(x, y));

        inputText.setFont(font);
        inputText.setCharacterSize(24);
        inputText.setFillColor(sf::Color::Black);
        inputText.setPosition(x + 5, y + 5);
        inputText.setString(s);
        userInput = s;

        sf::Event::KeyEvent keyEvent;
        textEnteredFunction = [this, keyEvent](const sf::Event& event) {
            if (event.type == sf::Event::TextEntered) {
                if (userInput.length() < 20 && isFocused && event.text.unicode >= 32 && event.text.unicode < 127) {
                    userInput += static_cast<char>(event.text.unicode);
                    inputText.setString(userInput);
                } else if (isFocused && event.text.unicode == 8 && !userInput.empty()) {
                    userInput.pop_back();
                    inputText.setString(userInput);
                }
            }
        };
    }

    void draw() {
        window.draw(inputBox);
        window.draw(inputText);
    }

    void handleEvent(const sf::Event& event) {
        if (event.type == sf::Event::MouseButtonPressed) {
            sf::Vector2i mousePos = sf::Mouse::getPosition(window);
            if (position.getGlobalBounds().contains(static_cast<float>(mousePos.x), static_cast<float>(mousePos.y))) {
                isFocused = true;
            } else {
                isFocused = false;
            }
        }

        if (event.type == sf::Event::LostFocus) {
            isFocused = false;
        }

        textEnteredFunction(event);
    }

    const std::string& getInput() const {
        return userInput;
    }

    void setPosition(int x, int y){
        inputBox.setPosition(x, y);
        inputText.setPosition(x+5, y+5);
    }

    void setText(string s){
        inputText.setString(s);
        userInput = s;
    }

private:
    sf::RenderWindow& window;
    sf::Font& font;
    sf::RectangleShape inputBox;
    sf::RectangleShape position;
    sf::Text inputText;
    bool isFocused;
    std::string userInput;
    std::function<void(const sf::Event&)> textEnteredFunction;
};

void funcion_regla(vector<bool>& current, vector<bool>& next, int cell){
    int regla_actual = 0;
    int j = 2 * neighbours;
    for(int i = neighbours; i>0 ; i--){
        if(cell-i < 0){
            regla_actual += pow(2, j--) * current[longitud_cadena-i];
        }
        else {
            regla_actual += pow(2, j--) * current[cell-i];
        }
    }
    regla_actual += pow(2, j--) * current[cell];
    for(int i = 1; i<=neighbours ; i++){
        if(cell+i > longitud_cadena-1){
            regla_actual += pow(2, j--) * current[cell+i-longitud_cadena];
        }
        else {
            regla_actual += pow(2, j--) * current[cell+i];
        }
    }

    mtx.lock();

    next[cell] = reglas[regla_actual];
    ones += reglas[regla_actual];
    veces_regla[regla_actual]++;

    mtx.unlock();

}

void crear_reglas(){
    int rule = regla;
    int size = pow(2, 1 + (2 * neighbours));
    veces_regla.resize(size, 0);
    reglas.resize(size, 0);
    for(int i = size-1; i >= 0; i--){
        if(rule < pow(2, i)){
            reglas[i] = 0;
        }
        else{
            reglas[i] = 1;
            rule -= pow(2, i);
        }
    }
}

void transformar(vector<bool>& a, vector<bool>& b, int start, int end){
    for(; start<end; start++){
        funcion_regla(ref(a), ref(b), start);
    }
}

void crear_hilos(vector<bool>& a, vector<bool>& b) {
    int num_threads = 12;
    int range = longitud_cadena / num_threads;
    vector<thread> threads;

    for (int i = 0; i < num_threads; i++) {
        int start = i * range;
        int end = (i == num_threads - 1) ? longitud_cadena : (i + 1) * range;

        threads.emplace_back(transformar, ref(a), ref(b), start, end);
    }

    for (auto& th : threads) {
        th.join();
    }
}

double calcular_varianza(const vector<bool>& gen1, const vector<bool>& gen2, float prev_mean, float mean){
    double varianza = 0;
    for(int i=0; i<longitud_cadena; i++){
        int diff = gen1[i] - gen2[i];
        varianza += pow(gen1[i] - gen2[i] - (prev_mean - mean), 2);
    }
    return varianza/longitud_cadena;
}

void plot(){
    gnuplot p;
    string command, name;
    vector<string> plots = {"densidad", "log10densidad", "entropia", "media", "varianza"};
    //Formato
    p("set term png");
    //Alcance
    command = "set xrange [0:";
    command += to_string(numRows);
    command += "]";
    p(command);
    //Nombre
    name += "regla";
    name += to_string(regla);
    name += "prob";
    name += to_string(probability);
    for(string s: plots){
        //Nombre
        command = "set output \"C:/CA/graficas/";
        command += name;
        command += s;
        command += ".png\"";
        p(command);
        //Titulo
        command = "set title \"";
        command += name;
        command += "\"";
        p(command);
        //rango
        command = "set yrange[0:";
        if(s == "densidad"){
            command += to_string(numRows + (numRows/10));
        } else if(s == "log10densidad"){
            command += "5";
        } else if(s == "media"){
            command += "1.5";
        } else if(s == "varianza"){
            command += "1";
        } else if(s == "entropia"){
            command += "3";
        }
        command += "]";
        p(command);
        //plot
        command = "plot \"C:/CA/plots/";
        command += s;
        command += ".txt\" using ($0+1):1 title \"";
        command += s;
        command += "\" pointtype 2 lt rgb \"blue\"";
        p(command);
    }
}

int main() {

    std::srand(seed);
    crear_reglas();
    sf::RenderWindow window(sf::VideoMode::getDesktopMode(), "CA Lab", sf::Style::Fullscreen);
    sf::View view;
    int render_height;
    int render_width;
    bool is_menu_open = 0;
    bool updateGrid = false;
    bool all = false;
    int gridSize = 3;
    float scrollSpeed = 20.0f;
    int top_center = window.getSize().y / 2;
    int left_center = window.getSize().x / 2;
    numRows = window.getSize().y / gridSize;
    longitud_cadena = window.getSize().x / gridSize;
    sf::Font font;
    font.loadFromFile("C:\\fonts\\roboto.ttf");

    string user_cols;

    //GNUPLOT
    gnuplot p;

    vector<bool> first(longitud_cadena, 0);
    vector<bool> current(longitud_cadena, 0);
    vector<bool> next(longitud_cadena, 0);

    TextInputBox input_rule(window, font, 220, 260, 200, 40, to_string(regla));
    TextInputBox input_prob(window, font, 220, 320, 200, 40, to_string(probability));
    TextInputBox input_cols(window, font, 220, 380, 200, 40, to_string(longitud_cadena));
    TextInputBox input_rows(window, font, 220, 440, 200, 40, to_string(numRows));
    TextInputBox input_size(window, font, 220, 500, 200, 40, to_string(gridSize));
    TextInputBox input_save(window, font, 430, 560, 200, 40, "default");
    TextInputBox input_load(window, font, 220, 620, 200, 40, "default");

    //Botones
    Button random(window, "Random", sf::Vector2f(10, 20), sf::Vector2f(200, 40));
    random.setOnClick([&]() {
        for(int i = 0; i<longitud_cadena; i++){
            first[i] = rand() % 100 < probability;
        }
    });

    Button middle(window, "Middle", sf::Vector2f(10, 80), sf::Vector2f(200, 40));
    middle.setOnClick([&]() {
        for(int i = 0; i<longitud_cadena; i++){
            first[i] = 0;
        }
        first[longitud_cadena/2] = 1;
    });

    Button alive_color(window, "Alive Color", sf::Vector2f(10, 140), sf::Vector2f(200, 40));
    alive_color.border.setSize(sf::Vector2f (210, 50));
    alive_color.border.setPosition(sf::Vector2f(5, 135));
    alive_color.color = sf::Color::Black;
    alive_color.mouseover = sf::Color::Black;

    Button dead_color(window, "Dead Color", sf::Vector2f(10, 200), sf::Vector2f(200, 40));
    dead_color.border.setSize(sf::Vector2f (210, 50));
    dead_color.border.setPosition(sf::Vector2f(5, 195));
    dead_color.color = sf::Color::Black;
    dead_color.mouseover = sf::Color::Black;

    Button get_rule(window, "Set rule", sf::Vector2f(10, 260), sf::Vector2f(200, 40));
    get_rule.setOnClick([&]() {
        try{
            regla = stoi(input_rule.getInput());
            if(regla < 0){
                regla = 0;
            }
            else if(regla > 255){
                regla = 255;
            }
            crear_reglas();
        }
        catch (exception &err){
            cout << "Regla debe ser numero" << endl;
        }
        input_rule.setText(to_string(regla));
    });

    Button get_prob(window, "Set probability", sf::Vector2f(10, 320), sf::Vector2f(200, 40));
    get_prob.setOnClick([&]() {
        try{
            probability = stoi(input_prob.getInput());
            if(probability < 0){
                probability = 0;
            }
            else if(probability > 100){
                probability = 100;
            }
        }
        catch (exception &err){
            cout << "Probabilidad debe ser numero" << endl;
        }
        input_prob.setText(to_string(probability));
    });

    Button get_cols(window, "Set columns", sf::Vector2f(10, 380), sf::Vector2f(200, 40));
    get_cols.setOnClick([&]() {
        try {
            longitud_cadena = stoi(input_cols.getInput());
            if(longitud_cadena <= 0){
                longitud_cadena = 0;
            }
            else if(longitud_cadena * gridSize > 32768){
                longitud_cadena -= ceil((longitud_cadena * gridSize - 32768) / gridSize);
                while(longitud_cadena * gridSize > 32768){
                    longitud_cadena--;
                }
            }
            first.resize(longitud_cadena, 0);
            current.resize(longitud_cadena, 0);
            next.resize(longitud_cadena, 0);
        } catch (exception& err) {
            cout << "Columnas debe ser numero" << endl;
        }
        input_cols.setText(to_string(longitud_cadena));
    });

    Button get_rows(window, "Set rows", sf::Vector2f(10, 440), sf::Vector2f(200, 40));
    get_rows.setOnClick([&]() {
        try {
            numRows = stoi(input_rows.getInput());
            if(numRows <= 0){
                numRows = 0;
            }
            else if(numRows * gridSize > 32768){
                numRows -= ceil((numRows * gridSize - 32768) / gridSize);
                while(numRows * gridSize > 32768){
                    numRows--;
                }
            }
        } catch (exception& err) {
            cout << "Renglones debe ser numero" << endl;
        }
        input_rows.setText(to_string(numRows));
    });

    Button get_gridsize(window, "Set size", sf::Vector2f(10, 500), sf::Vector2f(200, 40));
    get_gridsize.setOnClick([&]() {
        try {
            gridSize = stoi(input_size.getInput());
            if(gridSize <= 0) {
                gridSize = 1;
            }
            else if(gridSize >= 20){
                gridSize = 19;
            }
        } catch (exception& err) {
            cout << "Gridsize debe ser numero" << endl;
        }
        input_size.setText(to_string(gridSize));
    });

    Button save_first(window, "Save first config", sf::Vector2f(10, 560), sf::Vector2f(200, 40));
    save_first.setOnClick([&]() {
        string s = input_save.getInput();
        if(s != ""){
            s.insert(0, "C:\\CA\\saves\\");
            s += ".txt";
            ofstream save(s);
            for(auto x: first){
                save << to_string(x);
            }
            save.close();
        }
    });

    Button save_last(window, "Save last config", sf::Vector2f(220, 560), sf::Vector2f(200, 40));
    save_last.setOnClick([&]() {
        string s = input_save.getInput();
        if(s != ""){
            s.insert(0, "C:\\CA\\saves\\");
            s += ".txt";
            ofstream save(s);
            for(auto x: next){
                save << to_string(x);
            }
            save.close();
        }
    });

    Button load(window, "Load config", sf::Vector2f(10, 620), sf::Vector2f(200, 40));
    load.setOnClick([&]() {
        string s = input_load.getInput();
        if(s != ""){
            s.insert(0, "C:\\CA\\saves\\");
            s += ".txt";
            ifstream save(s, ios_base::in);
            string line;
            getline(save, line);
            first.clear();
            for(int i=0; i<line.size(); i++){
                first.push_back(line[i] - '0');
            }
            longitud_cadena = first.size();

            current.resize(longitud_cadena, 0);
            next.resize(longitud_cadena, 0);
            save.close();
        }
    });

    Button exito(window, "Exit", sf::Vector2f(window.getSize().x-220, 20), sf::Vector2f(200, 40));
    exito.setOnClick([&]() {
        window.close();
    });

    Button alive_red(window, "", sf::Vector2f(230, 140), sf::Vector2f(40, 40));
    alive_red.setOnClick([&]() {
        alive = sf::Color::Red;
    });
    alive_red.color = sf::Color::Red;
    alive_red.mouseover = sf::Color::Red;

    Button alive_blue(window, "", sf::Vector2f(280, 140), sf::Vector2f(40, 40));
    alive_blue.setOnClick([&]() {
        alive = sf::Color::Blue;
    });
    alive_blue.color = sf::Color::Blue;
    alive_blue.mouseover = sf::Color::Blue;

    Button alive_yellow(window, "", sf::Vector2f(330, 140), sf::Vector2f(40, 40));
    alive_yellow.setOnClick([&]() {
        alive = sf::Color::Yellow;
    });
    alive_yellow.color = sf::Color::Yellow;
    alive_yellow.mouseover = sf::Color::Yellow;

    Button alive_green(window, "", sf::Vector2f(380, 140), sf::Vector2f(40, 40));
    alive_green.setOnClick([&]() {
        alive = sf::Color::Green;
    });
    alive_green.color = sf::Color::Green;
    alive_green.mouseover = sf::Color::Green;

    Button alive_magenta(window, "", sf::Vector2f(430, 140), sf::Vector2f(40, 40));
    alive_magenta.setOnClick([&]() {
        alive = sf::Color::Magenta;
    });
    alive_magenta.color = sf::Color::Magenta;
    alive_magenta.mouseover = sf::Color::Magenta;

    Button alive_white(window, "", sf::Vector2f(480, 140), sf::Vector2f(40, 40));
    alive_white.setOnClick([&]() {
        alive = sf::Color::White;
    });
    alive_white.color = sf::Color::White;
    alive_white.mouseover = sf::Color::White;

    Button alive_black(window, "", sf::Vector2f(530, 140), sf::Vector2f(40, 40));
    alive_black.setOnClick([&]() {
        alive = sf::Color::Black;
    });
    alive_black.color = sf::Color::Black;
    alive_black.mouseover = sf::Color::Black;
    alive_black.border_color = sf::Color::White;

    Button dead_red(window, "", sf::Vector2f(230, 200), sf::Vector2f(40, 40));
    dead_red.setOnClick([&]() {
        dead = sf::Color::Red;
    });
    dead_red.color = sf::Color::Red;
    dead_red.mouseover = sf::Color::Red;

    Button dead_blue(window, "", sf::Vector2f(280, 200), sf::Vector2f(40, 40));
    dead_blue.setOnClick([&]() {
        dead = sf::Color::Blue;
    });
    dead_blue.color = sf::Color::Blue;
    dead_blue.mouseover = sf::Color::Blue;

    Button dead_yellow(window, "", sf::Vector2f(330, 200), sf::Vector2f(40, 40));
    dead_yellow.setOnClick([&]() {
        dead = sf::Color::Yellow;
    });
    dead_yellow.color = sf::Color::Yellow;
    dead_yellow.mouseover = sf::Color::Yellow;

    Button dead_green(window, "", sf::Vector2f(380, 200), sf::Vector2f(40, 40));
    dead_green.setOnClick([&]() {
        dead = sf::Color::Green;
    });
    dead_green.color = sf::Color::Green;
    dead_green.mouseover = sf::Color::Green;

    Button dead_magenta(window, "", sf::Vector2f(430, 200), sf::Vector2f(40, 40));
    dead_magenta.setOnClick([&]() {
        dead = sf::Color::Magenta;
    });
    dead_magenta.color = sf::Color::Magenta;
    dead_magenta.mouseover = sf::Color::Magenta;

    Button dead_white(window, "", sf::Vector2f(480, 200), sf::Vector2f(40, 40));
    dead_white.setOnClick([&]() {
        dead = sf::Color::White;
    });
    dead_white.color = sf::Color::White;
    dead_white.mouseover = sf::Color::White;

    Button dead_black(window, "", sf::Vector2f(530, 200), sf::Vector2f(40, 40));
    dead_black.setOnClick([&]() {
        dead = sf::Color::Black;
    });
    dead_black.color = sf::Color::Black;
    dead_black.mouseover = sf::Color::Black;
    dead_black.border_color = sf::Color::White;

    sf::RenderTexture renderTexture;
    renderTexture.create(window.getSize().x, window.getSize().y);
    renderTexture.clear(dead);

    view.setSize(window.getSize().x, window.getSize().y);
    view.setCenter(window.getSize().x / 2.0f, window.getSize().y / 2.0f);
    window.setView(view);

    auto updateAndDrawGrid = [&current, &next, &gridSize, &renderTexture, &render_height, &render_width]() {
        int varianza = 0;
        render_width = gridSize * longitud_cadena;
        render_height = gridSize * numRows;
        renderTexture.create(render_width, render_height);
        renderTexture.clear(dead);
        ofstream density("C:\\CA\\plots\\densidad.txt");
        ofstream log_density("C:\\CA\\plots\\log10densidad.txt");
        ofstream entropy("C:\\CA\\plots\\entropia.txt");
        ofstream mean("C:\\CA\\plots\\media.txt");
        ofstream variance("C:\\CA\\plots\\varianza.txt");
        for (int i = 1; i < numRows; ++i) {
            //GNUPLOT densidad a 0
            ones = 0;
            for(int i=0; i<veces_regla.size(); i++){
                veces_regla[i] = 0;
            }

            crear_hilos(current, next);
            for (int j = 0; j < longitud_cadena; ++j) {
                sf::RectangleShape cell(sf::Vector2f(gridSize, gridSize));
                cell.setPosition(j * gridSize, i * gridSize);
                cell.setFillColor(next[j] ? alive : dead);
                renderTexture.draw(cell);
            }


            //GNUPLOT escribir a los archivos
            float average = float(ones)/longitud_cadena;
            double H = 0;
            for(int Q: veces_regla){
                H += ((float(Q)/float(longitud_cadena))*(log2(float(Q)/float(longitud_cadena))));
            }
            H = -H;
            if(density.is_open()){
                density << ones << "\n";
            }
            if(log_density.is_open()){
                log_density << log10(ones) << "\n";
            }
            if(entropy.is_open()){
                if(H != H) H = 0;
                entropy << H << "\n";
            }
            if(mean.is_open()){
                mean << average << "\n";
            }
            if(variance.is_open()){
                variance << calcular_varianza(current, next, prev_mean, average) << "\n";
            }
            prev_mean = average;
            current = next;
        }

        density.close();
        log_density.close();
        entropy.close();
        mean.close();
        variance.close();
        renderTexture.display();

        if(true){
            string render = "C:/CA/renders/";
            render += to_string(regla);
            render += "p";
            render += to_string(probability);
            render += ".png";
            renderTexture.getTexture().copyToImage().saveToFile(render);
        }

    };


    while (window.isOpen()) {
        sf::Event event;
        while (window.pollEvent(event)) {
            if (event.type == sf::Event::Closed) {
                window.close();
            }

            input_rule.handleEvent(event);
            input_prob.handleEvent(event);
            input_cols.handleEvent(event);
            input_rows.handleEvent(event);
            input_size.handleEvent(event);
            input_save.handleEvent(event);
            input_load.handleEvent(event);

            if (event.type == sf::Event::KeyPressed) {
                if (event.key.code == sf::Keyboard::Escape) {
                    is_menu_open = !is_menu_open; // Close the window when the "Esc" key is pressed
                }
                if (event.key.code == sf::Keyboard::Right or event.key.code == sf::Keyboard::Left
                    or event.key.code == sf::Keyboard::Up or event.key.code == sf::Keyboard::Down){
                    sf::Vector2f center = view.getCenter();

                    // Update the center position based on the scroll direction
                    if (event.key.code == sf::Keyboard::Left) {
                        center.x -= 10 * gridSize;
                    } else if (event.key.code == sf::Keyboard::Right) {
                        center.x += 10 * gridSize;
                    } else if (event.key.code == sf::Keyboard::Up) {
                        center.y -= 10 * gridSize;
                    } else if (event.key.code == sf::Keyboard::Down) {
                        center.y += 10 * gridSize;
                    }

                    sf::FloatRect viewBounds = view.getViewport();
                    if (center.x < left_center or render_width < window.getSize().x) {
                        center.x = left_center;
                    } else if (center.x > render_width - left_center) {
                        center.x = render_width - left_center;
                    }
                    if (center.y < top_center or render_height < window.getSize().y) {
                        center.y = top_center;
                    } else if (center.y > render_height - top_center) {
                        center.y = render_height - top_center;
                    }

                    int y_update = center.y - top_center;
                    int x_update = center.x - left_center;

                    exito.setPosition(window.getSize().x - 220 + x_update, 20 + y_update);
                    random.setPosition(10 + x_update, 20 + y_update);
                    middle.setPosition(10 + x_update, 80 + y_update);
                    alive_color.setPosition(10 + x_update, 140 + y_update);
                    alive_color.border.setPosition(5 + x_update, 135 + y_update);
                    dead_color.setPosition(10 + x_update, 200 + y_update);
                    dead_color.border.setPosition(5 + x_update, 195 + y_update);
                    get_rule.setPosition(10 + x_update, 260 + y_update);
                    input_rule.setPosition(220 + x_update, 260 + y_update);
                    get_prob.setPosition(10 + x_update, 320 + y_update);
                    input_prob.setPosition(220 + x_update, 320 + y_update);
                    get_cols.setPosition(10 + x_update, 380 + y_update);
                    input_cols.setPosition(220 + x_update, 380 + y_update);
                    save_first.setPosition(10 + x_update, 560 + y_update);
                    save_last.setPosition(220 + x_update, 560 + y_update);
                    input_save.setPosition(430 + x_update, 560 + y_update);
                    alive_red.setPosition(230 + x_update, 140 + y_update);
                    alive_blue.setPosition(280 + x_update, 140 + y_update);
                    alive_yellow.setPosition(330 + x_update, 140 + y_update);
                    alive_green.setPosition(380 + x_update, 140 + y_update);
                    alive_magenta.setPosition(430 + x_update, 140 + y_update);
                    alive_white.setPosition(480 + x_update, 140 + y_update);
                    alive_black.setPosition(530 + x_update, 140 + y_update);
                    dead_red.setPosition(230 + x_update, 200 + y_update);
                    dead_blue.setPosition(280 + x_update, 200 + y_update);
                    dead_yellow.setPosition(330 + x_update, 200 + y_update);
                    dead_green.setPosition(380 + x_update, 200 + y_update);
                    dead_magenta.setPosition(430 + x_update, 200 + y_update);
                    dead_white.setPosition(480 + x_update, 200 + y_update);
                    dead_black.setPosition(530 + x_update, 200 + y_update);
                    get_rows.setPosition(10 + x_update, 440 + y_update);
                    input_rows.setPosition(220 + x_update, 440 + y_update);
                    get_gridsize.setPosition(10 + x_update, 500 + y_update);
                    input_size.setPosition(220 + x_update, 500 + y_update);
                    load.setPosition(10 + x_update, 620 + y_update);
                    input_load.setPosition(220 + x_update, 620 + y_update);

                    view.setCenter(center);
                    window.setView(view);
                }
                if (event.key.code == sf::Keyboard::Enter) {
                    view.setCenter(left_center, top_center);
                    exito.setPosition(window.getSize().x - 220, 20);
                    random.setPosition(10, 20);
                    middle.setPosition(10, 80);
                    alive_color.setPosition(10, 140);
                    alive_color.border.setPosition(5, 135);
                    dead_color.setPosition(10, 200);
                    dead_color.border.setPosition(5, 195);
                    get_rule.setPosition(10, 260);
                    input_rule.setPosition(220, 260);
                    get_prob.setPosition(10, 320);
                    input_prob.setPosition(220, 320);
                    get_cols.setPosition(10, 380);
                    input_cols.setPosition(220, 380);
                    save_first.setPosition(10, 560);
                    save_last.setPosition(220, 560);
                    input_save.setPosition(430, 560);
                    alive_red.setPosition(230, 140);
                    alive_blue.setPosition(280, 140);
                    alive_yellow.setPosition(330, 140);
                    alive_green.setPosition(380, 140);
                    alive_magenta.setPosition(430, 140);
                    alive_white.setPosition(480, 140);
                    alive_black.setPosition(530, 140);
                    dead_red.setPosition(230, 200);
                    dead_blue.setPosition(280, 200);
                    dead_yellow.setPosition(330, 200);
                    dead_green.setPosition(380, 200);
                    dead_magenta.setPosition(430, 200);
                    dead_white.setPosition(480, 200);
                    dead_black.setPosition(530, 200);
                    get_rows.setPosition(10, 440);
                    input_rows.setPosition(220, 440);
                    get_gridsize.setPosition(10, 500);
                    input_size.setPosition(220, 500);
                    load.setPosition(10, 620);
                    input_load.setPosition(220, 620);
                    updateGrid = 1;
                    window.setView(view);
                }
                if (event.key.code == sf::Keyboard::F1) {
                    view.setCenter(left_center, top_center);
                    exito.setPosition(window.getSize().x - 220, 20);
                    random.setPosition(10, 20);
                    middle.setPosition(10, 80);
                    alive_color.setPosition(10, 140);
                    alive_color.border.setPosition(5, 135);
                    dead_color.setPosition(10, 200);
                    dead_color.border.setPosition(5, 195);
                    get_rule.setPosition(10, 260);
                    input_rule.setPosition(220, 260);
                    get_prob.setPosition(10, 320);
                    input_prob.setPosition(220, 320);
                    get_cols.setPosition(10, 380);
                    input_cols.setPosition(220, 380);
                    save_first.setPosition(10, 560);
                    save_last.setPosition(220, 560);
                    input_save.setPosition(430, 560);
                    alive_red.setPosition(230, 140);
                    alive_blue.setPosition(280, 140);
                    alive_yellow.setPosition(330, 140);
                    alive_green.setPosition(380, 140);
                    alive_magenta.setPosition(430, 140);
                    alive_white.setPosition(480, 140);
                    alive_black.setPosition(530, 140);
                    dead_red.setPosition(230, 200);
                    dead_blue.setPosition(280, 200);
                    dead_yellow.setPosition(330, 200);
                    dead_green.setPosition(380, 200);
                    dead_magenta.setPosition(430, 200);
                    dead_white.setPosition(480, 200);
                    dead_black.setPosition(530, 200);
                    get_rows.setPosition(10, 440);
                    input_rows.setPosition(220, 440);
                    get_gridsize.setPosition(10, 500);
                    input_size.setPosition(220, 500);
                    load.setPosition(10, 620);
                    input_load.setPosition(220, 620);
                    all = 1;
                    window.setView(view);
                }
            }

            if (event.type == sf::Event::MouseButtonPressed) {
                sf::Vector2i mousePos = sf::Mouse::getPosition(window);
                int col = mousePos.x / gridSize;
                int row = mousePos.y / gridSize;

                if (row == 0 and event.mouseButton.button == sf::Mouse::Left) {
                    first[col] = !first[col];
                }
            }
            if (event.type == sf::Event::MouseWheelScrolled) {
                sf::Vector2f center = view.getCenter();

                if (event.mouseWheelScroll.delta > 0) {
                    center.y -= scrollSpeed;
                } else if (event.mouseWheelScroll.delta < 0) {
                    center.y += scrollSpeed;
                }

                if (center.y < top_center or render_height < window.getSize().y) {
                    center.y = top_center;
                } else if (center.y > render_height - top_center) {
                    center.y = render_height - top_center;
                }

                int y_update = center.y - top_center;
                int x_update = center.x - left_center;

                exito.setPosition(window.getSize().x - 220 + x_update, 20 + y_update);
                random.setPosition(10 + x_update, 20 + y_update);
                middle.setPosition(10 + x_update, 80 + y_update);
                alive_color.setPosition(10 + x_update, 140 + y_update);
                alive_color.border.setPosition(5 + x_update, 135 + y_update);
                dead_color.setPosition(10 + x_update, 200 + y_update);
                dead_color.border.setPosition(5 + x_update, 195 + y_update);
                get_rule.setPosition(10 + x_update, 260 + y_update);
                input_rule.setPosition(220 + x_update, 260 + y_update);
                get_prob.setPosition(10 + x_update, 320 + y_update);
                input_prob.setPosition(220 + x_update, 320 + y_update);
                get_cols.setPosition(10 + x_update, 380 + y_update);
                input_cols.setPosition(220 + x_update, 380 + y_update);
                save_first.setPosition(10 + x_update, 560 + y_update);
                save_last.setPosition(220 + x_update, 560 + y_update);
                input_save.setPosition(430 + x_update, 560 + y_update);
                alive_red.setPosition(230 + x_update, 140 + y_update);
                alive_blue.setPosition(280 + x_update, 140 + y_update);
                alive_yellow.setPosition(330 + x_update, 140 + y_update);
                alive_green.setPosition(380 + x_update, 140 + y_update);
                alive_magenta.setPosition(430 + x_update, 140 + y_update);
                alive_white.setPosition(480 + x_update, 140 + y_update);
                alive_black.setPosition(530 + x_update, 140 + y_update);
                dead_red.setPosition(230 + x_update, 200 + y_update);
                dead_blue.setPosition(280 + x_update, 200 + y_update);
                dead_yellow.setPosition(330 + x_update, 200 + y_update);
                dead_green.setPosition(380 + x_update, 200 + y_update);
                dead_magenta.setPosition(430 + x_update, 200 + y_update);
                dead_white.setPosition(480 + x_update, 200 + y_update);
                dead_black.setPosition(530 + x_update, 200 + y_update);
                get_rows.setPosition(10 + x_update, 440 + y_update);
                input_rows.setPosition(220 + x_update, 440 + y_update);
                get_gridsize.setPosition(10 + x_update, 500 + y_update);
                input_size.setPosition(220 + x_update, 500 + y_update);
                load.setPosition(10 + x_update, 620 + y_update);
                input_load.setPosition(220 + x_update, 620 + y_update);

                view.setCenter(center);
                window.setView(view);
            }
        }


        if (all) {
            vector<int> eq_rules = {0,1,2,3,4,5,6,7,8,9,10,11,12,13,14,15,18,19,22,23,24,25,26,27,28,29,30,32,33,34,35,36,37,38,40,41,42,43,44,45,46,50,51,54,
                                    56,57,58,60,62,72,73,74,76,77,78,90,94,104,105,106,108,110,122,126,128,130,132,134,136,138,140,142,146,150,152,154,156,160,162,164,168,170,172,178,184,200,204,232};
            for(auto r: eq_rules){
                regla = r;
                crear_reglas();
                current = first;
                updateAndDrawGrid();
                updateGrid = false;
                plot();
                window.clear();

                sf::Sprite sprite(renderTexture.getTexture());
                window.draw(sprite);
                window.display();
            }
            all = false;
        }

        if (updateGrid) {
            current = first;
            updateAndDrawGrid();
            updateGrid = false;
            plot();
        }

        window.clear();

        sf::Sprite sprite(renderTexture.getTexture());
        window.draw(sprite);

        for (int i = 0; i < longitud_cadena; ++i) {
            sf::RectangleShape cell(sf::Vector2f(gridSize, gridSize));
            cell.setPosition(i * gridSize, 0);
            cell.setFillColor(first[i] ? alive : dead);
            window.draw(cell);
        }

        if(is_menu_open){
            alive_color.border_color = alive;
            dead_color.border_color = dead;
            exito.update();
            random.update();
            middle.update();
            alive_color.update();
            dead_color.update();
            get_rule.update();
            get_prob.update();
            get_cols.update();
            get_gridsize.update();
            save_first.update();
            save_last.update();
            alive_red.update();
            alive_blue.update();
            alive_yellow.update();
            alive_green.update();
            alive_magenta.update();
            alive_white.update();
            alive_black.update();
            dead_red.update();
            dead_blue.update();
            dead_yellow.update();
            dead_green.update();
            dead_magenta.update();
            dead_white.update();
            dead_black.update();
            get_rows.update();
            load.update();
            random.draw();
            middle.draw();
            alive_color.draw();
            dead_color.draw();
            get_rule.draw();
            input_rule.draw();
            get_prob.draw();
            input_prob.draw();
            get_cols.draw();
            input_cols.draw();
            get_gridsize.draw();
            input_size.draw();
            save_first.draw();
            save_last.draw();
            input_save.draw();
            load.draw();
            input_load.draw();
            exito.draw();
            get_rows.draw();
            input_rows.draw();

            alive_red.draw();
            alive_blue.draw();
            alive_yellow.draw();
            alive_green.draw();
            alive_magenta.draw();
            alive_white.draw();
            alive_black.draw();

            dead_red.draw();
            dead_blue.draw();
            dead_yellow.draw();
            dead_green.draw();
            dead_magenta.draw();
            dead_white.draw();
            dead_black.draw();
        }

        window.display();
    }

    return 0;
}
