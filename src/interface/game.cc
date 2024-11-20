#include <optional>
#include <random>
#include <thread>
#include <mutex>
#include <shared_mutex>

#include "../../src/interface/game.hpp"

#include "../../src/game/interface.hpp"
#include "../../src/game/sprite.hpp"
#include "../../src/game/tile.hpp"

#include "../../src/game/symbols/car.hpp"
#include "../../src/game/symbols/frog.hpp"
#include "../../src/game/symbols/lily.hpp"

std::shared_mutex mutex;

struct NODE {
    mutable int index;
    mutable bool active;
    mutable bool pause = false;
    mutable std::thread worker;
    mutable SPRITE sprite;
    mutable TILE tile;
    mutable NODE* previous = nullptr;
    mutable NODE* next = nullptr;
};

auto total(NODE* &root, const std::optional<TYPE> &type = std::nullopt) -> int {
    int index = 0;

    {
        std::shared_lock<std::shared_mutex> lock(mutex);
        for (const NODE *current = root; current != nullptr; current = current->next) {
            if (type.has_value()) {
                if (type.value() == current->tile.type) {
                    index++;
                }
            } else {
                index++;
            }

            if (!current->next) {
                break;
            }
        }
    }

    return index;
}

auto create(NODE* &root, NODE* &previous, const TILE &tile, const SPRITE &sprite, const bool &active = true) -> NODE* {
    for (NODE* current = root; current != nullptr; current = current->next) {
        if (current == previous) {
            if (!current->next && !previous->next) {
                current->next = new NODE {
                    .index = current->index + 1,
                    .active = active,
                    .sprite = sprite,
                    .tile = tile,
                    .previous = previous,
                };
            }
        }
    }

    return previous->next;
}

void animate(const NODE* root, const INTERFACE &context) {
    while (root->active) {
        switch (root->tile.type) {
            case RIGHT_CAR:
                if (root->tile.board.x <= context.visual.width) {
                    {
                        std::unique_lock<std::shared_mutex> lock(mutex);
                        root->tile.board.x = root->tile.board.x + root->sprite.width;
                    }

                    if (root->tile.board.x >= context.visual.width) {
                        {
                            std::unique_lock<std::shared_mutex> lock(mutex);
                            root->tile.board.x = 0;
                        }
                    }
                }
                break;
            case LEFT_CAR:
                if (root->tile.board.x >= 0) {
                    {
                        std::unique_lock<std::shared_mutex> lock(mutex);
                        root->tile.board.x = root->tile.board.x - root->sprite.width;
                    }

                    if (root->tile.board.x < 0) {
                        {
                            std::unique_lock<std::shared_mutex> lock(mutex);
                            root->tile.board.x = context.visual.width - root->sprite.width;
                        }
                    }
                }
                break;
            default:
                break;
        }

#if DEBUG
        std::this_thread::sleep_for(std::chrono::seconds(1));
#else
        std::this_thread::sleep_for(std::chrono::milliseconds(125));
#endif
    }
}

void play(NODE* &root, WINDOW* &window, const INTERFACE &context, const CONFIGURATION &configuration) {
    std::random_device seed;
    std::mt19937 generate(seed());
    std::uniform_int_distribution<int> distribution(1, configuration.environment.car);

    auto last = create(root, root, tile(SEPARATOR, context.visual.height - 1 - root->sprite.height - 1, 0), {});

    int initial = total(root);

    for (size_t i = 1; i <= 4; i++) {
        for (NODE* current = root; current != nullptr; current = current->next) {
            const int now = total(root);

            if (current->index < now) {
                continue;
            }

            const int random = distribution(generate);

            if (initial != now) {
                initial = now;
                break;
            }

            if (!current->next) {
                if (i % 2 != 0) {
                    const SPRITE rcar = sprite(RCAR);

                    {
                        std::unique_lock<std::shared_mutex> lock(mutex);
                        for (int j = now + 1; j <= now + random; j++) {
                            switch (last->tile.type) {
                                case SEPARATOR:
                                    last = create(last, last, tile(RIGHT_CAR, last->tile.board.y - rcar.height, 0), rcar, false);
                                    break;
                                case RIGHT_CAR:
                                    last = create(last, last, tile(RIGHT_CAR, last->tile.board.y, last->tile.board.x + rcar.width), rcar, false);
                                    break;
                                case LEFT_CAR:
                                    last = create(last, last, tile(RIGHT_CAR, last->tile.board.y - rcar.height - 1, 0), rcar, false);
                                    break;
                                default:
                                    exit(1);
                            }
                        }
                    }
                } else {
                    const SPRITE lcar = sprite(LCAR);

                    {
                        std::unique_lock<std::shared_mutex> lock(mutex);
                        for (int j = now + 1; j <= now + random; j++) {
                            switch (last->tile.type) {
                                case SEPARATOR:
                                    last = create(last, last, tile(LEFT_CAR, last->tile.board.y - lcar.height, context.visual.width - lcar.width), lcar, false);
                                    break;
                                case RIGHT_CAR:
                                    last = create(last, last, tile(LEFT_CAR, last->tile.board.y - lcar.height - 1, context.visual.width), lcar, false);
                                    break;
                                case LEFT_CAR:
                                    last = create(last, last, tile(LEFT_CAR, last->tile.board.y, last->tile.board.x - lcar.width), lcar, false);
                                    break;
                                default:
                                    exit(1);
                            }
                        }
                    }
                }
            }
        }
    }

    last = create(root, last, tile(SEPARATOR, last->tile.board.y - 1, 0), {});
    last = create(root, last, tile(SEPARATOR, last->tile.board.y - root->sprite.height - 1, 0), {});

    auto previous = last;

    for (size_t i = 1; i <= 5 * root->sprite.height; i++) {
        last = create(root, last, tile(WATER, last->tile.board.y - 1, 0), {});
    }

    const SPRITE x = sprite(LILY_1);
    const SPRITE y = sprite(LILY_2);
    const SPRITE z = sprite(LILY_3);

    for (size_t i = 1; i <= 3; i++) {
        for (size_t j = 1; j <= 3; j++) {
            for (NODE* current = root; current != nullptr; current = current->next) {
                const int now = total(root);

                if (current->index < now) {
                    continue;
                }

                if (initial != now) {
                    initial = now;
                    break;
                }

                if (!current->next) {
                    {
                        std::unique_lock<std::shared_mutex> lock(mutex);
                        auto lily = new NODE {
                            .index = current->index + 1,
                            .active = false,
                            .sprite = {
                                .symbol = x.symbol,
                                .height = x.height,
                                .width = x.width,
                                .next = new SPRITE {
                                    .symbol = y.symbol,
                                    .height = y.height,
                                    .width = y.width,
                                    .next = new SPRITE {
                                        .symbol = z.symbol,
                                        .height = z.height,
                                        .width = z.width,
                                    }
                                }
                            },
                            .previous = current,
                        };

                        if (j == 1) {
                            lily->tile = tile(LILLY, previous->tile.board.y - root->sprite.height - 1, x.width + x.width);
                        } else {
                            lily->tile = tile(LILLY, previous->tile.board.y - root->sprite.height - 1, current->tile.board.x + x.width);
                        }

                        current->next = lily;
                    }
                }
            }
        }
    }

    for (NODE* current = root->next; current != nullptr; current = current->next) {
        if (current->active == false && (current->tile.type != SEPARATOR || current->tile.type != PLAYER || current->tile.type != WATER)) {
            {
                std::unique_lock<std::shared_mutex> lock(mutex);
                current->active = true;
                current->worker = std::thread(animate, current, std::ref(context));
            }
        }
    }

    while (configuration.status.play) {
        if (const std::string interface = keypad(window, context, root->sprite, root->tile); interface == "q") {
            break;
        }

        /*
        for (const NODE* current = root->next; current != nullptr; current = current->next) {}
        */

        std::this_thread::sleep_for(std::chrono::milliseconds(1));
    }

    configuration.status.play = false;
}

void game(WINDOW* &parent, const CONFIGURATION &configuration) {
    const INTERFACE context = {
        .visual = {
            .y = 0,
            .x = 0,
            .height = getmaxy(stdscr),
            .width = getmaxx(stdscr),
        },
    };

    wclear(parent);
    wrefresh(parent);
    werase(parent);

    WINDOW *window = create(context.visual.y, context.visual.x, context.visual.height, context.visual.width);

    nodelay(window, true);
    keypad(window, true);
    clearok(window, true);

    {
        std::unique_lock<std::shared_mutex> lock(mutex);
        configuration.status.play = true;
    }

    const SPRITE frog = sprite(LFROG);
    const TILE player = tile(PLAYER, context.visual.height - frog.height - 1, context.visual.width / 2 + 1);

    auto root = new NODE {
        .index = 1,
        .active = true,
        .sprite = frog,
        .tile = player
    };

    std::thread playing(play, std::ref(root), std::ref(window), std::ref(context), std::ref(configuration));

    while (configuration.status.running) {
        wclear(window);

        wbkgd(window, ' ' | A_NORMAL);

        werase(window);

        {
            std::shared_lock<std::shared_mutex> lock(mutex);

            if (configuration.status.play == false) {
                break;
            }

            for (const NODE *current = root; current != nullptr; current = current->next) {
                if (current->active == true) {
                    switch (current->tile.type) {
                        case SEPARATOR:
                            for (size_t index = 0; index < context.visual.width; index++) {
                                mvwprintw(window, current->tile.board.y, index, "%c", '=');
                            }
                            break;
                        case RIGHT_CAR:
                            for (size_t index = 0; index < current->sprite.symbol.size(); index++) {
                                mvwprintw(window, current->tile.board.y + index, current->tile.board.x, "%s",
                                          current->sprite.symbol[index].c_str());
                            }
#if DEBUG
                            mvwprintw(window, current->tile.board.y + 2, current->tile.board.x + 6, "%s", std::to_string(current->tile.board.x).c_str());
                            mvwprintw(window, current->tile.board.y + 2, current->tile.board.x + 10, "%s", std::to_string(current->tile.board.y).c_str());
#endif
                            break;
                        case LEFT_CAR:
                            for (size_t index = 0; index < current->sprite.symbol.size(); index++) {
                                mvwprintw(window, current->tile.board.y + index, current->tile.board.x, "%s",
                                          current->sprite.symbol[index].c_str());
                            }
#if DEBUG
                            mvwprintw(window, current->tile.board.y + 2, current->tile.board.x + 6, "%s", std::to_string(current->tile.board.x).c_str());
                            mvwprintw(window, current->tile.board.y + 2, current->tile.board.x + 10, "%s", std::to_string(current->tile.board.y).c_str());
#endif
                            break;
#if RELEASE
                        case WATER:
                            for (size_t index = 0; index < context.visual.width; index++) {
                                mvwprintw(window, current->tile.board.y, index, "%c", '~');
                            }
                            break;
#endif
                        case LILLY:
                            for (size_t index = 0; index < current->sprite.symbol.size(); index++) {
                                mvwprintw(window, current->tile.board.y + index, current->tile.board.x, "%s",
                                          current->sprite.symbol[index].c_str());
                            }
                            break;
                        default:
                            break;
                    }
                }
            }

            for (size_t index = 0; index < root->sprite.symbol.size(); index++) {
                mvwprintw(window, root->tile.board.y + index, root->tile.board.x, "%s",
                          root->sprite.symbol[index].c_str());
            }

            mvwprintw(window, context.visual.height - 1, 0, "%s", "ATAS = KEY UP");
            mvwprintw(window, context.visual.height - 1, 15, "%s", "BAWAH = KEY DOWN");
            mvwprintw(window, context.visual.height - 1, 35, "%s", "KANAN = KEY RIGHT");
            mvwprintw(window, context.visual.height - 1, 55, "%s", "KIRI = KEY LEFT");

#if DEBUG
            const int right = total(root, RIGHT_CAR);

            mvwprintw(window, context.visual.height - 1, 100, "%s", "RIGHT CAR = ");
            mvwprintw(window, context.visual.height - 1, 115, "%s", std::to_string(right).c_str());

            const int left = total(root, LEFT_CAR);

            mvwprintw(window, context.visual.height - 1, 125, "%s", "LEFT CAR = ");
            mvwprintw(window, context.visual.height - 1, 145, "%s", std::to_string(left).c_str());

            mvwprintw(window, context.visual.height - 1, 155, "%s", "X = ");
            mvwprintw(window, context.visual.height - 1, 160, "%s", std::to_string(root->tile.board.x).c_str());

            mvwprintw(window, context.visual.height - 1, 165, "%s", "Y = ");
            mvwprintw(window, context.visual.height - 1, 170, "%s", std::to_string(root->tile.board.y).c_str());
#endif
        }

        wnoutrefresh(window);

#if DEBUG
        std::this_thread::sleep_for(std::chrono::seconds(1));
#else
        std::this_thread::sleep_for(std::chrono::milliseconds(1));
#endif

        doupdate();

        wrefresh(window);
    }

    for (const NODE* current = root; current != nullptr; current = current->next) {
        if (current->active == true && (current->tile.type == RIGHT_CAR || current->tile.type == LEFT_CAR || current->tile.type == LILLY)) {
            {
                std::unique_lock<std::shared_mutex> lock(mutex);
                current->active = false;
            }

            current->worker.join();
        }
    }

    playing.join();

    wclear(window);
    wclrtoeol(window);
    wrefresh(window);
    werase(window);
    delwin(window);

    delete root;
}
