#include <SFML/Graphics.hpp>

int juan() {
    sf::RenderWindow window(sf::VideoMode(800, 600), "SFML Grid Example");

    int gridSize = 20;
    int numRows = window.getSize().y / gridSize;
    int numCols = window.getSize().x / gridSize;

    // Initialize the grid as a 2D array of booleans (on/off state)
    bool grid[numCols][numRows] = {false};

    while (window.isOpen()) {
        sf::Event event;
        while (window.pollEvent(event)) {
            if (event.type == sf::Event::Closed) {
                window.close();
            }

            // Handle mouse input
            if (event.type == sf::Event::MouseButtonPressed) {
                sf::Vector2i mousePos = sf::Mouse::getPosition(window);
                int col = mousePos.x / gridSize;
                int row = mousePos.y / gridSize;

                if (event.mouseButton.button == sf::Mouse::Left) {
                    grid[col][row] = !grid[col][row];
                }
            }
        }

        window.clear();

        // Draw the grid based on the grid data
        for (int i = 0; i < numCols; ++i) {
            for (int j = 0; j < numRows; ++j) {
                sf::RectangleShape cell(sf::Vector2f(gridSize, gridSize));
                cell.setPosition(i * gridSize, j * gridSize);
                cell.setFillColor(grid[i][j] ? sf::Color::Green : sf::Color::White);
                window.draw(cell);
            }
        }

        window.display();
    }

    return 0;
}
