#include <SFML/Graphics.hpp>
#include <vector>
#include <cmath>
#include <string>
#include <optional>

using namespace std;

bool touch(const sf::FloatRect& a, const sf::FloatRect& b)
{
    return a.findIntersection(b).has_value();
}

class Entity
{
protected:
    sf::RectangleShape body;
    sf::Vector2f velocity;

public:
    virtual ~Entity() {}

    virtual void update(float dt) = 0;

    virtual void draw(sf::RenderWindow& window)
    {
        window.draw(body);
    }

    sf::FloatRect getBounds() const
    {
        return body.getGlobalBounds();
    }

    sf::Vector2f getPosition() const
    {
        return body.getPosition();
    }
};

enum class PlatformType
{
    Normal,
    Moving,
    Breakable
};

class Platform
{
private:
    sf::RectangleShape shape;
    PlatformType type;

    float moveOffset;
    bool movingRight;
    bool broken;

public:
    Platform(float x, float y, float w, float h, PlatformType t)
    {
        shape.setPosition({x, y});
        shape.setSize({w, h});

        type = t;
        moveOffset = 0.f;
        movingRight = true;
        broken = false;

        if (type == PlatformType::Normal)
            shape.setFillColor(sf::Color(90, 90, 90));

        if (type == PlatformType::Moving)
            shape.setFillColor(sf::Color(70, 120, 200));

        if (type == PlatformType::Breakable)
            shape.setFillColor(sf::Color(130, 90, 60));
    }

    void update(float dt)
    {
        if (broken)
            return;

        if (type == PlatformType::Moving)
        {
            if (movingRight)
            {
                shape.move({100.f * dt, 0.f});
                moveOffset += 100.f * dt;

                if (moveOffset > 220.f)
                    movingRight = false;
            }
            else
            {
                shape.move({-100.f * dt, 0.f});
                moveOffset -= 100.f * dt;

                if (moveOffset < 0.f)
                    movingRight = true;
            }
        }
    }

    void draw(sf::RenderWindow& window)
    {
        if (!broken)
            window.draw(shape);
    }

    sf::FloatRect getBounds() const
    {
        return shape.getGlobalBounds();
    }

    PlatformType getType() const
    {
        return type;
    }

    bool isBroken() const
    {
        return broken;
    }

    void breakPlatform()
    {
        broken = true;
    }
};

enum class TrapType
{
    Spikes,
    Fire,
    MovingSaw,
    FallingBlock
};

class Trap : public Entity
{
private:
    TrapType type;
    float baseX;
    float baseY;
    float moveTimer;
    bool falling;

public:
    Trap(float x, float y, TrapType t)
    {
        type = t;
        baseX = x;
        baseY = y;
        moveTimer = 0.f;
        falling = false;

        body.setPosition({x, y});
        velocity = {0.f, 0.f};

        if (type == TrapType::Spikes)
        {
            body.setSize({60.f, 30.f});
            body.setFillColor(sf::Color(180, 180, 180));
        }

        if (type == TrapType::Fire)
        {
            body.setSize({40.f, 60.f});
            body.setFillColor(sf::Color(255, 100, 20));
        }

        if (type == TrapType::MovingSaw)
        {
            body.setSize({50.f, 50.f});
            body.setFillColor(sf::Color::White);
        }

        if (type == TrapType::FallingBlock)
        {
            body.setSize({60.f, 60.f});
            body.setFillColor(sf::Color(110, 100, 90));
        }
    }

    void activateFalling()
    {
        if (type == TrapType::FallingBlock)
            falling = true;
    }

    TrapType getType() const
    {
        return type;
    }

    void update(float dt) override
    {
        if (type == TrapType::MovingSaw)
        {
            moveTimer += dt;
            float x = baseX + sin(moveTimer * 2.f) * 180.f;
            body.setPosition({x, baseY});
        }

        if (type == TrapType::Fire)
        {
            moveTimer += dt;
        }

        if (type == TrapType::FallingBlock && falling)
        {
            velocity.y += 900.f * dt;
            body.move({0.f, velocity.y * dt});
        }
    }

    void draw(sf::RenderWindow& window) override
    {
        if (type == TrapType::Spikes)
        {
            for (int i = 0; i < 5; i++)
            {
                sf::CircleShape spike(9.f, 3);
                spike.setRotation(sf::degrees(180.f));
                spike.setFillColor(sf::Color(190, 190, 190));
                spike.setPosition({
                    body.getPosition().x + i * 12.f,
                    body.getPosition().y + 18.f
                });
                window.draw(spike);
            }

            return;
        }

        if (type == TrapType::Fire)
        {
            sf::RectangleShape flame;
            flame.setSize({40.f, 50.f});
            flame.setPosition(body.getPosition());

            int pulse = static_cast<int>((sin(moveTimer * 8.f) + 1.f) * 40.f);
            flame.setFillColor(sf::Color(220 + pulse / 2, 70 + pulse, 10));
            window.draw(flame);

            sf::CircleShape top(18.f, 3);
            top.setFillColor(sf::Color(255, 160, 40));
            top.setRotation(sf::degrees(180.f));
            top.setPosition({body.getPosition().x + 2.f, body.getPosition().y - 6.f});
            window.draw(top);

            return;
        }

        if (type == TrapType::MovingSaw)
        {
            sf::CircleShape saw(25.f, 12);
            saw.setPosition(body.getPosition());
            saw.setFillColor(sf::Color(210, 210, 210));
            saw.rotate(sf::degrees(moveTimer * 200.f));
            window.draw(saw);

            return;
        }

        window.draw(body);
    }
};

class Coin : public Entity
{
private:
    bool collected;
    float timer;

public:
    Coin(float x, float y)
    {
        collected = false;
        timer = 0.f;

        body.setSize({20.f, 20.f});
        body.setFillColor(sf::Color::Yellow);
        body.setPosition({x, y});
    }

    void update(float dt) override
    {
        timer += dt;
        body.setScale({1.f + sin(timer * 6.f) * 0.15f, 1.f});
        body.rotate(sf::degrees(120.f * dt));
    }

    bool isCollected() const
    {
        return collected;
    }

    void collect()
    {
        collected = true;
    }
};

class Enemy : public Entity
{
private:
    int hp;
    float speed;
    bool alive;
    float hurtTimer;

public:
    Enemy(float x, float y)
    {
        body.setPosition({x, y});
        body.setSize({40.f, 40.f});
        body.setFillColor(sf::Color(120, 220, 120));

        velocity = {0.f, 0.f};

        hp = 2;
        speed = 120.f;
        alive = true;
        hurtTimer = 0.f;
    }

    void updateAI(sf::Vector2f playerPos)
    {
        if (!alive)
            return;

        float dx = playerPos.x - body.getPosition().x;

        if (abs(dx) < 360.f)
        {
            if (dx < 0)
                velocity.x = -speed;
            else
                velocity.x = speed;
        }
        else
        {
            velocity.x = 0.f;
        }
    }

    void update(float dt) override
    {
        if (!alive)
            return;

        if (hurtTimer > 0.f)
            hurtTimer -= dt;

        body.move({velocity.x * dt, velocity.y * dt});
    }

    void draw(sf::RenderWindow& window) override
    {
        if (!alive)
            return;

        sf::CircleShape slime(22.f);

        if (hurtTimer > 0.f)
            slime.setFillColor(sf::Color::Red);
        else
            slime.setFillColor(sf::Color(120, 220, 120));

        slime.setPosition(body.getPosition());
        window.draw(slime);

        sf::CircleShape eye(3.f);
        eye.setFillColor(sf::Color::Black);

        eye.setPosition({
            body.getPosition().x + 10.f,
            body.getPosition().y + 10.f
        });
        window.draw(eye);

        eye.setPosition({
            body.getPosition().x + 25.f,
            body.getPosition().y + 10.f
        });
        window.draw(eye);
    }

    bool isAlive() const
    {
        return alive;
    }

    void takeDamage()
    {
        if (!alive)
            return;

        hp--;
        hurtTimer = 0.15f;

        if (hp <= 0)
            alive = false;
    }
};

class Player : public Entity
{
private:
    bool onGround;
    bool canDoubleJump;
    bool canDash;
    bool attacking;
    bool faceRight;

    int hp;
    int coins;

    float speed;
    float gravity;
    float jumpForce;

    float attackTimer;
    float damageTimer;
    float dashTimer;

    sf::RectangleShape dagger;

public:
    Player()
    {
        body.setSize({40.f, 60.f});
        body.setFillColor(sf::Color(180, 140, 70));
        body.setPosition({100.f, 300.f});

        velocity = {0.f, 0.f};

        onGround = false;
        canDoubleJump = true;
        canDash = true;
        attacking = false;
        faceRight = true;

        hp = 5;
        coins = 0;

        speed = 320.f;
        gravity = 1400.f;
        jumpForce = -650.f;

        attackTimer = 0.f;
        damageTimer = 0.f;
        dashTimer = 0.f;

        dagger.setSize({34.f, 8.f});
        dagger.setFillColor(sf::Color(230, 230, 240));
    }

    void handleInput()
    {
        velocity.x = 0.f;

        if (sf::Keyboard::isKeyPressed(sf::Keyboard::Key::A))
        {
            velocity.x = -speed;
            faceRight = false;
        }

        if (sf::Keyboard::isKeyPressed(sf::Keyboard::Key::D))
        {
            velocity.x = speed;
            faceRight = true;
        }

        if (sf::Keyboard::isKeyPressed(sf::Keyboard::Key::LShift))
        {
            dash();
        }

        if (sf::Keyboard::isKeyPressed(sf::Keyboard::Key::F))
        {
            attack();
        }
    }

    void jump()
    {
        if (onGround)
        {
            velocity.y = jumpForce;
            onGround = false;
            canDoubleJump = true;
        }
        else if (canDoubleJump)
        {
            velocity.y = jumpForce;
            canDoubleJump = false;
        }
    }

    void dash()
    {
        if (!canDash)
            return;

        canDash = false;
        dashTimer = 0.14f;

        if (faceRight)
            velocity.x = 900.f;
        else
            velocity.x = -900.f;

        velocity.y = 0.f;
    }

    void attack()
    {
        if (attackTimer <= 0.f)
        {
            attacking = true;
            attackTimer = 0.18f;
        }
    }

    bool isAttacking() const
    {
        return attacking;
    }

    sf::FloatRect getAttackBounds() const
    {
        sf::RectangleShape hitbox;
        hitbox.setSize({45.f, 28.f});

        if (faceRight)
        {
            hitbox.setPosition({
                body.getPosition().x + 38.f,
                body.getPosition().y + 15.f
            });
        }
        else
        {
            hitbox.setPosition({
                body.getPosition().x - 43.f,
                body.getPosition().y + 15.f
            });
        }

        return hitbox.getGlobalBounds();
    }

    void applyGravity(float dt)
    {
        velocity.y += gravity * dt;
    }

    void resolvePlatformCollision(const sf::FloatRect& platform)
    {
        sf::FloatRect playerBounds = body.getGlobalBounds();

        float playerBottom = playerBounds.position.y + playerBounds.size.y;
        float platformTop = platform.position.y;

        if (playerBottom <= platformTop + 35.f && velocity.y > 0)
        {
            body.setPosition({
                body.getPosition().x,
                platformTop - playerBounds.size.y
            });

            velocity.y = 0.f;
            onGround = true;
            canDash = true;
        }
    }

    void update(float dt) override
    {
        handleInput();

        if (attackTimer > 0.f)
        {
            attackTimer -= dt;
            attacking = true;
        }
        else
        {
            attacking = false;
        }

        if (damageTimer > 0.f)
            damageTimer -= dt;

        if (dashTimer > 0.f)
        {
            dashTimer -= dt;
        }
        else
        {
            applyGravity(dt);
        }

        body.move({velocity.x * dt, velocity.y * dt});

        if (abs(velocity.x) > 10.f)
            body.setScale({1.f, 0.95f});
        else
            body.setScale({1.f, 1.f});

        if (!onGround)
            body.setScale({0.9f, 1.1f});

        if (body.getPosition().y > 900.f)
            hp = 0;
    }

    void draw(sf::RenderWindow& window) override
    {
        sf::Vector2f pos = body.getPosition();

        sf::RectangleShape legs;
        legs.setSize({36.f, 18.f});
        legs.setFillColor(sf::Color(70, 50, 45));
        legs.setPosition({pos.x + 2.f, pos.y + 42.f});
        window.draw(legs);

        sf::RectangleShape coat;
        coat.setSize({40.f, 45.f});

        if (damageTimer > 0.f)
            coat.setFillColor(sf::Color::Red);
        else
            coat.setFillColor(sf::Color(150, 100, 60));

        coat.setPosition(pos);
        window.draw(coat);

        sf::CircleShape head(12.f);
        head.setFillColor(sf::Color(220, 180, 140));
        head.setPosition({pos.x + 8.f, pos.y - 20.f});
        window.draw(head);

        sf::RectangleShape hood;
        hood.setSize({26.f, 10.f});
        hood.setFillColor(sf::Color(80, 50, 30));
        hood.setPosition({pos.x + 7.f, pos.y - 15.f});
        window.draw(hood);

        sf::RectangleShape belt;
        belt.setSize({42.f, 6.f});
        belt.setFillColor(sf::Color(45, 30, 20));
        belt.setPosition({pos.x - 1.f, pos.y + 25.f});
        window.draw(belt);

        if (attacking)
        {
            if (faceRight)
            {
                dagger.setPosition({pos.x + 40.f, pos.y + 22.f});
                dagger.setRotation(sf::degrees(0.f));
            }
            else
            {
                dagger.setPosition({pos.x - 35.f, pos.y + 22.f});
                dagger.setRotation(sf::degrees(0.f));
            }

            window.draw(dagger);
        }
    }

    void takeDamage(int dmg)
    {
        if (damageTimer > 0.f)
            return;

        hp -= dmg;
        damageTimer = 0.7f;
    }

    int getHP() const
    {
        return hp;
    }

    int getCoins() const
    {
        return coins;
    }

    void addCoin()
    {
        coins++;
    }
};

int main()
{
    sf::RenderWindow window(
        sf::VideoMode({1280u, 720u}),
        "Forgotten Dungeon"
    );

    window.setFramerateLimit(60);

    Player player;

    sf::View camera;
    camera.setSize({1280.f, 720.f});
    camera.setCenter({640.f, 360.f});

    vector<Platform> platforms;
    vector<Enemy> enemies;
    vector<Trap> traps;
    vector<Coin> coins;

    bool gameOver = false;
    bool victory = false;

    float timeScale = 1.f;

    platforms.push_back(Platform(0, 650, 2600, 80, PlatformType::Normal));
    platforms.push_back(Platform(400, 520, 220, 30, PlatformType::Normal));
    platforms.push_back(Platform(720, 460, 180, 30, PlatformType::Moving));
    platforms.push_back(Platform(1050, 390, 180, 30, PlatformType::Breakable));
    platforms.push_back(Platform(1350, 520, 240, 30, PlatformType::Normal));
    platforms.push_back(Platform(1750, 450, 200, 30, PlatformType::Moving));
    platforms.push_back(Platform(2100, 560, 260, 30, PlatformType::Normal));

    enemies.push_back(Enemy(700, 600));
    enemies.push_back(Enemy(1450, 600));
    enemies.push_back(Enemy(2000, 510));

    traps.push_back(Trap(1120, 620, TrapType::Spikes));
    traps.push_back(Trap(1540, 590, TrapType::Fire));
    traps.push_back(Trap(1780, 600, TrapType::MovingSaw));
    traps.push_back(Trap(1900, 260, TrapType::FallingBlock));

    coins.push_back(Coin(450, 480));
    coins.push_back(Coin(760, 420));
    coins.push_back(Coin(1080, 350));
    coins.push_back(Coin(1400, 480));
    coins.push_back(Coin(2150, 520));

    sf::Font font;
    bool hasFont = font.openFromFile("/System/Library/Fonts/Helvetica.ttc");

    sf::Clock clock;

    while (window.isOpen())
    {
        float dt = clock.restart().asSeconds();

        while (const optional<sf::Event> event = window.pollEvent())
        {
            if (event->is<sf::Event::Closed>())
            {
                window.close();
            }

            if (const auto* key = event->getIf<sf::Event::KeyPressed>())
            {
                if (key->code == sf::Keyboard::Key::Space)
                {
                    player.jump();
                }

                if (key->code == sf::Keyboard::Key::Q)
                {
                    timeScale = 0.35f;
                }
            }

            if (const auto* key = event->getIf<sf::Event::KeyReleased>())
            {
                if (key->code == sf::Keyboard::Key::Q)
                {
                    timeScale = 1.f;
                }
            }
        }

        if (!gameOver && !victory)
        {
            float scaledDt = dt * timeScale;

            player.update(dt);

            sf::Vector2f playerPos = player.getPosition();

            if (playerPos.x < 640.f)
                camera.setCenter({640.f, 360.f});
            else if (playerPos.x > 1960.f)
                camera.setCenter({1960.f, 360.f});
            else
                camera.setCenter({playerPos.x, 360.f});

            for (auto& p : platforms)
            {
                p.update(scaledDt);

                if (!p.isBroken() && touch(player.getBounds(), p.getBounds()))
                {
                    player.resolvePlatformCollision(p.getBounds());

                    if (p.getType() == PlatformType::Breakable)
                        p.breakPlatform();
                }
            }

            for (auto& e : enemies)
            {
                e.updateAI(player.getPosition());
                e.update(scaledDt);

                if (e.isAlive() && touch(player.getBounds(), e.getBounds()))
                {
                    player.takeDamage(1);
                }

                if (e.isAlive() && player.isAttacking() && touch(player.getAttackBounds(), e.getBounds()))
                {
                    e.takeDamage();
                }
            }

            for (auto& t : traps)
            {
                if (t.getType() == TrapType::FallingBlock)
                {
                    float dx = abs(player.getPosition().x - t.getPosition().x);

                    if (dx < 90.f)
                        t.activateFalling();
                }

                t.update(scaledDt);

                if (touch(player.getBounds(), t.getBounds()))
                {
                    player.takeDamage(1);
                }
            }

            for (auto& c : coins)
            {
                c.update(dt);

                if (!c.isCollected() && touch(player.getBounds(), c.getBounds()))
                {
                    c.collect();
                    player.addCoin();
                }
            }

            if (player.getHP() <= 0)
                gameOver = true;

            if (player.getPosition().x > 2320.f)
                victory = true;
        }

        window.clear(sf::Color(25, 20, 15));

        window.setView(camera);

        sf::RectangleShape background;
        background.setSize({4000.f, 1000.f});
        background.setFillColor(sf::Color(30, 25, 20));
        window.draw(background);

        for (int i = 0; i < 25; i++)
        {
            sf::CircleShape crystal(10.f, 4);
            crystal.setRotation(sf::degrees(45.f));
            crystal.setFillColor(sf::Color(120, 0, 200));
            crystal.setPosition({
                180.f * i,
                160.f + static_cast<float>((i % 4) * 90)
            });
            window.draw(crystal);
        }

        for (int i = 0; i < 10; i++)
        {
            sf::RectangleShape column;
            column.setSize({35.f, 300.f});
            column.setFillColor(sf::Color(45, 38, 34));
            column.setPosition({250.f + i * 300.f, 350.f});
            window.draw(column);
        }

        for (auto& p : platforms)
            p.draw(window);

        for (auto& t : traps)
            t.draw(window);

        for (auto& e : enemies)
            e.draw(window);

        for (auto& c : coins)
        {
            if (!c.isCollected())
                c.draw(window);
        }

        player.draw(window);

        sf::RectangleShape treasureBase;
        treasureBase.setSize({70.f, 45.f});
        treasureBase.setFillColor(sf::Color(180, 100, 20));
        treasureBase.setPosition({2380.f, 605.f});
        window.draw(treasureBase);

        sf::RectangleShape treasureGold;
        treasureGold.setSize({58.f, 15.f});
        treasureGold.setFillColor(sf::Color(255, 215, 0));
        treasureGold.setPosition({2386.f, 590.f});
        window.draw(treasureGold);

        window.setView(window.getDefaultView());

        for (int i = 0; i < player.getHP(); i++)
        {
            sf::CircleShape heart(12.f);
            heart.setFillColor(sf::Color::Red);
            heart.setPosition({20.f + i * 35.f, 20.f});
            window.draw(heart);
        }

        if (hasFont)
        {
            sf::Text coinText(font, "Coins: " + to_string(player.getCoins()), 28);
            coinText.setFillColor(sf::Color::White);
            coinText.setPosition({20.f, 60.f});
            window.draw(coinText);

            sf::Text abilityText(font, "A/D move | Space jump | Shift dash | F dagger | Q slow time", 20);
            abilityText.setFillColor(sf::Color(220, 220, 220));
            abilityText.setPosition({20.f, 100.f});
            window.draw(abilityText);

            if (gameOver)
            {
                sf::Text text(font, "YOU DIED", 64);
                text.setFillColor(sf::Color::Red);
                text.setPosition({420.f, 300.f});
                window.draw(text);
            }

            if (victory)
            {
                sf::Text text(font, "TREASURE FOUND", 56);
                text.setFillColor(sf::Color::Yellow);
                text.setPosition({320.f, 300.f});
                window.draw(text);
            }
        }

        window.display();
    }

    return 0;
}