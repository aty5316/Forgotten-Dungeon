#include <SFML/Graphics.hpp>
#include <vector>
#include <cmath>
#include <string>
#include <optional>
#include <sstream>
#include <iomanip>
#include <filesystem>
#include <iostream>
#include <cctype>

using namespace std;

const float TILE = 48.f;

string twoDigits(int value)
{
    ostringstream out;
    out << setw(2) << setfill('0') << value;
    return out.str();
}

bool touch(const sf::FloatRect& a, const sf::FloatRect& b)
{
    return a.findIntersection(b).has_value();
}

class FrameAnimation
{
private:
    vector<sf::Texture> frames;
    float timer;
    float fps;
    int current;
    bool loaded;

public:
    FrameAnimation()
    {
        timer = 0.f;
        fps = 10.f;
        current = 0;
        loaded = false;
    }

    bool loadSequence(const string& folder, const string& prefix, int count, float framesPerSecond)
    {
        frames.clear();
        timer = 0.f;
        current = 0;
        fps = framesPerSecond;

        for (int i = 0; i < count; i++)
        {
            string path = folder + "/" + prefix + "-" + twoDigits(i) + ".png";

            sf::Texture texture;
            if (!texture.loadFromFile(path))
            {
                frames.clear();
                loaded = false;
                return false;
            }

            texture.setSmooth(false);
            frames.push_back(std::move(texture));
        }

        loaded = !frames.empty();
        return loaded;
    }

    void restart()
    {
        timer = 0.f;
        current = 0;
    }

    void update(float dt, bool loop)
    {
        if (!loaded || frames.empty())
            return;

        timer += dt;
        float frameTime = 1.f / fps;

        while (timer >= frameTime)
        {
            timer -= frameTime;

            if (current + 1 < static_cast<int>(frames.size()))
                current++;
            else if (loop)
                current = 0;
            else
                current = static_cast<int>(frames.size()) - 1;
        }
    }

    bool isLoaded() const
    {
        return loaded;
    }

    int index() const
    {
        return current;
    }

    void draw(
        sf::RenderWindow& window,
        sf::Vector2f bottomCenter,
        bool faceRight,
        float scale,
        sf::Color color = sf::Color::White
    ) const
    {
        if (!loaded || frames.empty())
            return;

        const sf::Texture& texture = frames[current];
        sf::Sprite sprite(texture);

        sf::Vector2u size = texture.getSize();
        sprite.setOrigin({
            static_cast<float>(size.x) / 2.f,
            static_cast<float>(size.y)
        });

        if (faceRight)
            sprite.setScale({scale, scale});
        else
            sprite.setScale({-scale, scale});

        sprite.setPosition(bottomCenter);
        sprite.setColor(color);

        window.draw(sprite);
    }
};

class TileSet
{
private:
    sf::Texture texture;
    bool loaded;

public:
    TileSet()
    {
        loaded = texture.loadFromFile("CavePlatformerTileset/cave-platformer-tileset-16x16.png");

        if (loaded)
            texture.setSmooth(false);
    }

    bool isLoaded() const
    {
        return loaded;
    }

    void drawTile(sf::RenderWindow& window, int col, int row, float x, float y, float scale = 3.f) const
    {
        if (!loaded)
            return;

        sf::Sprite sprite(texture);
        sprite.setTextureRect(sf::IntRect({col * 16, row * 16}, {16, 16}));
        sprite.setPosition({x, y});
        sprite.setScale({scale, scale});
        window.draw(sprite);
    }

    void drawTileCustom(
        sf::RenderWindow& window,
        int col,
        int row,
        float x,
        float y,
        float sx,
        float sy
    ) const
    {
        if (!loaded)
            return;

        sf::Sprite sprite(texture);
        sprite.setTextureRect(sf::IntRect({col * 16, row * 16}, {16, 16}));
        sprite.setPosition({x, y});
        sprite.setScale({sx, sy});
        window.draw(sprite);
    }
};


class WorldBackground
{
private:
    sf::Texture texture;
    bool loaded;
    string loadedPath;

    bool tryLoadExact(const string& path)
    {
        if (!filesystem::exists(path))
            return false;

        sf::Texture candidate;

        if (!candidate.loadFromFile(path))
            return false;

        sf::Vector2u size = candidate.getSize();

        if (size.x < 900 || size.x < size.y * 2)
            return false;

        texture = std::move(candidate);
        loaded = true;
        loadedPath = path;
        cout << "Background size: " << size.x << "x" << size.y << endl;
        return true;
    }

    bool isPng(const filesystem::path& path) const
    {
        string ext = path.extension().string();

        for (char& c : ext)
            c = static_cast<char>(tolower(c));

        return ext == ".png";
    }

    bool looksLikeBackgroundName(string name) const
    {
        for (char& c : name)
            c = static_cast<char>(tolower(c));

        if (name.find("tileset") != string::npos)
            return false;

        return name.find("background") != string::npos ||
               name.find("cave_background") != string::npos ||
               name.find("cave") != string::npos ||
               name.find("фон") != string::npos ||
               name.find("пещ") != string::npos ||
               name.find("темн") != string::npos ||
               name.find("crystal") != string::npos ||
               name.find("generated") != string::npos;
    }

public:
    WorldBackground()
    {
        loaded = false;

        vector<string> exactPaths = {
            "CavePlatformerTileset/cave_background.png",
            "CavePlatformerTileset/background.png",
            "CavePlatformerTileset/CaveBackground.png",
            "CavePlatformerTileset/темная_пещера_с_голубыми_кристаллами.png",
            "assets/cave_background.png",
            "assets/background.png",
            "cave_background.png",
            "background.png",
            "темная_пещера_с_голубыми_кристаллами.png"
        };

        for (const string& path : exactPaths)
        {
            if (tryLoadExact(path))
            {
                cout << "Loaded background: " << loadedPath << endl;
                return;
            }
        }

        vector<string> folders = {
            "CavePlatformerTileset",
            "assets",
            "."
        };

        for (const string& folder : folders)
        {
            if (!filesystem::exists(folder))
                continue;

            for (const auto& entry : filesystem::directory_iterator(folder))
            {
                if (!entry.is_regular_file())
                    continue;

                filesystem::path path = entry.path();

                if (!isPng(path))
                    continue;

                if (!looksLikeBackgroundName(path.filename().string()))
                    continue;

                if (tryLoadExact(path.string()))
                {
                    cout << "Loaded background: " << loadedPath << endl;
                    return;
                }
            }
        }

        cout << "Background image was not found. Fallback cave background is used." << endl;
    }

    bool isLoaded() const
    {
        return loaded;
    }

    void draw(sf::RenderWindow& window, float levelWidth, float windowHeight) const
    {
        if (!loaded)
            return;

        sf::Sprite sprite(texture);

        sf::Vector2u size = texture.getSize();

        float scaleX = levelWidth / static_cast<float>(size.x);
        float scaleY = windowHeight / static_cast<float>(size.y);

        sprite.setPosition({0.f, 0.f});
        sprite.setScale({scaleX, scaleY});

        window.draw(sprite);

        sf::RectangleShape visibilityTint;
        visibilityTint.setPosition({0.f, 0.f});
        visibilityTint.setSize({levelWidth, windowHeight});
        visibilityTint.setFillColor(sf::Color(35, 28, 45, 45));
        window.draw(visibilityTint);
    }
};

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
    bool breaking;
    float breakTimer;
    sf::Vector2f stablePosition;

public:
    Platform(float x, float y, float w, float h, PlatformType t)
    {
        shape.setPosition({x, y});
        shape.setSize({w, h});

        type = t;
        moveOffset = 0.f;
        movingRight = true;
        broken = false;
        breaking = false;
        breakTimer = 0.f;
        stablePosition = shape.getPosition();

        if (type == PlatformType::Normal)
            shape.setFillColor(sf::Color(82, 61, 68));

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

            return;
        }

        if (type == PlatformType::Breakable)
        {
            if (breaking)
            {
                breakTimer -= dt;

                float shakeOffset = sin(breakTimer * 120.f) * 3.5f;
                shape.setPosition({stablePosition.x + shakeOffset, stablePosition.y});

                if (breakTimer <= 0.f)
                {
                    broken = true;
                    breaking = false;
                    shape.setPosition(stablePosition);
                }
            }
            else
            {
                shape.setPosition(stablePosition);
            }
        }
    }

    void draw(sf::RenderWindow& window, const TileSet& tiles)
    {
        if (broken)
            return;

        if (!tiles.isLoaded())
        {
            window.draw(shape);
            return;
        }

        sf::Vector2f p = shape.getPosition();
        sf::Vector2f s = shape.getSize();

        int cols = static_cast<int>(ceil(s.x / TILE));
        int rows = static_cast<int>(ceil(s.y / TILE));

        for (int y = 0; y < rows; y++)
        {
            for (int x = 0; x < cols; x++)
            {
                float dx = p.x + x * TILE;
                float dy = p.y + y * TILE;

                if (type == PlatformType::Breakable)
                {
                    if (y == 0)
                        tiles.drawTile(window, x % 4, 8, dx, dy);
                    else
                        tiles.drawTile(window, x % 2, 9, dx, dy);

                    continue;
                }

                if (type == PlatformType::Moving)
                {
                    tiles.drawTile(window, 3 + (x % 2), 9, dx, dy);
                    continue;
                }

                if (y == 0)
                    tiles.drawTile(window, x % 4, 4, dx, dy);
                else if (y == rows - 1)
                    tiles.drawTile(window, x % 4, 7, dx, dy);
                else
                    tiles.drawTile(window, x % 4, 5 + (y % 2), dx, dy);
            }
        }
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

    bool isBreaking() const
    {
        return breaking;
    }

    void startBreaking()
    {
        if (type != PlatformType::Breakable || broken || breaking)
            return;

        stablePosition = shape.getPosition();
        breaking = true;
        breakTimer = 0.8f;
    }

    void breakPlatform()
    {
        broken = true;
        breaking = false;
        shape.setPosition(stablePosition);
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
    Trap(float x, float y, TrapType t, float width = 96.f)
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
            body.setSize({width, 44.f});
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

    void draw(sf::RenderWindow& window, const TileSet& tiles)
    {
        if (type == TrapType::Spikes)
        {
            if (tiles.isLoaded())
            {
                int count = static_cast<int>(ceil(body.getSize().x / TILE));

                for (int i = 0; i < count; i++)
                {
                    tiles.drawTile(window, 6, 10, body.getPosition().x + i * TILE, body.getPosition().y);
                }

                return;
            }

            for (int i = 0; i < static_cast<int>(body.getSize().x / 24.f); i++)
            {
                sf::CircleShape spike(12.f, 3);
                spike.setRotation(sf::degrees(180.f));
                spike.setFillColor(sf::Color(190, 190, 190));
                spike.setPosition({
                    body.getPosition().x + i * 24.f,
                    body.getPosition().y + 22.f
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

class Diamond : public Entity
{
private:
    bool collected;
    float timer;

public:
    Diamond(float x, float y)
    {
        collected = false;
        timer = 0.f;

        body.setSize({32.f, 32.f});
        body.setFillColor(sf::Color(50, 180, 255));
        body.setPosition({x, y});
    }

    void update(float dt) override
    {
        timer += dt;
        body.setScale({1.f + sin(timer * 6.f) * 0.15f, 1.f});
    }

    void draw(sf::RenderWindow& window, const TileSet& tiles)
    {
        if (collected)
            return;

        if (tiles.isLoaded())
        {
            float bounce = sin(timer * 4.f) * 4.f;
            tiles.drawTile(window, 2, 10, body.getPosition().x - 8.f, body.getPosition().y - 8.f + bounce, 3.f);
            return;
        }

        sf::CircleShape crystal(16.f, 4);
        crystal.setRotation(sf::degrees(45.f));
        crystal.setFillColor(sf::Color(40, 170, 255));
        crystal.setPosition(body.getPosition());
        window.draw(crystal);
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
    bool spritesReady;

    int hp;
    int diamonds;

    float speed;
    float gravity;
    float jumpForce;

    float attackTimer;
    float damageTimer;
    float dashTimer;

    FrameAnimation idleAnim;
    FrameAnimation runAnim;
    FrameAnimation jumpAnim;
    FrameAnimation punchAnim;

    sf::RectangleShape dagger;

public:
    Player()
    {
        body.setSize({40.f, 60.f});
        body.setFillColor(sf::Color::Transparent);
        body.setPosition({80.f, 560.f});

        velocity = {0.f, 0.f};

        onGround = false;
        canDoubleJump = true;
        canDash = true;
        attacking = false;
        faceRight = true;
        spritesReady = false;

        hp = 5;
        diamonds = 0;

        speed = 320.f;
        gravity = 1400.f;
        jumpForce = -650.f;

        attackTimer = 0.f;
        damageTimer = 0.f;
        dashTimer = 0.f;

        string mainFolder = "Adventurer all/Adventurer/Individual Sprites";
        string handFolder = "Adventurer all/Adventurer-Hand-Combat/Individual Sprites";

        bool idleLoaded = idleAnim.loadSequence(mainFolder, "adventurer-idle", 4, 7.f);
        bool runLoaded = runAnim.loadSequence(mainFolder, "adventurer-run", 6, 12.f);
        bool jumpLoaded = jumpAnim.loadSequence(mainFolder, "adventurer-jump", 4, 8.f);
        bool punchLoaded = punchAnim.loadSequence(handFolder, "adventurer-punch", 13, 28.f);

        spritesReady = idleLoaded && runLoaded && jumpLoaded && punchLoaded;

        dagger.setSize({34.f, 8.f});
        dagger.setFillColor(sf::Color(230, 230, 240));
    }

    void handleInput()
    {
        velocity.x = 0.f;

        if (sf::Keyboard::isKeyPressed(sf::Keyboard::Scancode::A))
        {
            velocity.x = -speed;
            faceRight = false;
        }

        if (sf::Keyboard::isKeyPressed(sf::Keyboard::Scancode::D))
        {
            velocity.x = speed;
            faceRight = true;
        }

        if (sf::Keyboard::isKeyPressed(sf::Keyboard::Scancode::LShift))
        {
            dash();
        }

        if (sf::Keyboard::isKeyPressed(sf::Keyboard::Scancode::F))
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
            attackTimer = 0.46f;
            punchAnim.restart();
        }
    }

    bool isAttacking() const
    {
        if (!attacking)
            return false;

        int frame = punchAnim.index();
        return frame >= 3 && frame <= 10;
    }

    sf::FloatRect getAttackBounds() const
    {
        sf::RectangleShape hitbox;
        hitbox.setSize({68.f, 42.f});

        if (faceRight)
        {
            hitbox.setPosition({
                body.getPosition().x + 32.f,
                body.getPosition().y + 8.f
            });
        }
        else
        {
            hitbox.setPosition({
                body.getPosition().x - 60.f,
                body.getPosition().y + 8.f
            });
        }

        return hitbox.getGlobalBounds();
    }

    void applyGravity(float dt)
    {
        velocity.y += gravity * dt;
    }

    bool resolvePlatformCollision(const sf::FloatRect& platform)
    {
        sf::FloatRect playerBounds = body.getGlobalBounds();

        float playerBottom = playerBounds.position.y + playerBounds.size.y;
        float platformTop = platform.position.y;

        if (playerBottom <= platformTop + 35.f && velocity.y >= 0)
        {
            body.setPosition({
                body.getPosition().x,
                platformTop - playerBounds.size.y
            });

            velocity.y = 0.f;
            onGround = true;
            canDash = true;
            return true;
        }

        return false;
    }

    void update(float dt) override
    {
        handleInput();

        onGround = false;
        body.setScale({1.f, 1.f});

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

        if (spritesReady)
        {
            if (attacking)
                punchAnim.update(dt, false);
            else if (!onGround)
                jumpAnim.update(dt, true);
            else if (abs(velocity.x) > 10.f)
                runAnim.update(dt, true);
            else
                idleAnim.update(dt, true);
        }

        if (body.getPosition().y > 900.f)
            hp = 0;
    }

    void drawFallback(sf::RenderWindow& window)
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

        if (attacking)
        {
            if (faceRight)
                dagger.setPosition({pos.x + 40.f, pos.y + 22.f});
            else
                dagger.setPosition({pos.x - 35.f, pos.y + 22.f});

            window.draw(dagger);
        }
    }

    void draw(sf::RenderWindow& window) override
    {
        if (!spritesReady)
        {
            drawFallback(window);
            return;
        }

        sf::Vector2f bottomCenter = {
            body.getPosition().x + body.getSize().x / 2.f,
            body.getPosition().y + body.getSize().y + 4.f
        };

        sf::Color tint = sf::Color::White;

        if (damageTimer > 0.f)
            tint = sf::Color(255, 120, 120);

        float spriteScale = 2.25f;

        if (attacking)
        {
            punchAnim.draw(window, bottomCenter, faceRight, spriteScale, tint);
        }
        else if (!onGround)
        {
            jumpAnim.draw(window, bottomCenter, faceRight, spriteScale, tint);
        }
        else if (abs(velocity.x) > 10.f)
        {
            runAnim.draw(window, bottomCenter, faceRight, spriteScale, tint);
        }
        else
        {
            idleAnim.draw(window, bottomCenter, faceRight, spriteScale, tint);
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

    int getDiamonds() const
    {
        return diamonds;
    }

    void addDiamond()
    {
        diamonds++;
    }
};

void drawBackground(sf::RenderWindow& window, const TileSet& tiles, const WorldBackground& backgroundImage, float levelWidth)
{
    if (backgroundImage.isLoaded())
    {
        backgroundImage.draw(window, levelWidth, 720.f);
    }
    else
    {
        sf::RectangleShape background;
        background.setSize({levelWidth + 1280.f, 1000.f});
        background.setFillColor(sf::Color(25, 19, 24));
        window.draw(background);

        sf::RectangleShape farWall;
        farWall.setSize({levelWidth + 1280.f, 440.f});
        farWall.setPosition({0.f, 120.f});
        farWall.setFillColor(sf::Color(50, 31, 45));
        window.draw(farWall);

        for (int i = 0; i < static_cast<int>(levelWidth / 180.f); i++)
        {
            sf::CircleShape crystalGlow(9.f, 4);
            crystalGlow.setRotation(sf::degrees(45.f));
            crystalGlow.setFillColor(sf::Color(38, 87, 130));
            crystalGlow.setPosition({
                120.f + i * 180.f,
                145.f + static_cast<float>((i % 5) * 58)
            });
            window.draw(crystalGlow);
        }
    }

    if (tiles.isLoaded())
    {
        for (int x = 0; x < static_cast<int>(levelWidth / TILE) + 4; x++)
        {
            tiles.drawTile(window, x % 4, 0, x * TILE, 0.f);
            tiles.drawTile(window, x % 4, 1, x * TILE, 48.f);
        }

        for (int i = 0; i < 11; i++)
        {
            float x = 300.f + i * 430.f;

            tiles.drawTile(window, 4, 8, x, 96.f);
            tiles.drawTile(window, 4, 9, x, 144.f);
            tiles.drawTile(window, 4, 10, x, 192.f);
        }

    }
}

int main()
{
    const float levelWidth = 5400.f;

    sf::RenderWindow window(
        sf::VideoMode({1280u, 720u}),
        "Forgotten Dungeon"
    );

    window.setFramerateLimit(60);

    TileSet tiles;
    WorldBackground backgroundImage;
    Player player;

    sf::View camera;
    camera.setSize({1280.f, 720.f});
    camera.setCenter({640.f, 360.f});

    vector<Platform> platforms;
    vector<Enemy> enemies;
    vector<Trap> traps;
    vector<Diamond> diamonds;

    bool gameOver = false;
    bool victory = false;

    float timeScale = 1.f;

    // Main cave road, variant 2: jump -> spikes -> double jump -> dash -> final climb.
    platforms.push_back(Platform(0, 620, 650, 96, PlatformType::Normal));
    platforms.push_back(Platform(830, 620, 340, 96, PlatformType::Normal));

    platforms.push_back(Platform(1240, 580, 280, 136, PlatformType::Normal));
    platforms.push_back(Platform(1570, 540, 300, 176, PlatformType::Normal));

    platforms.push_back(Platform(2140, 430, 360, 286, PlatformType::Normal));
    platforms.push_back(Platform(2600, 520, 280, 196, PlatformType::Normal));

    platforms.push_back(Platform(3040, 590, 280, 126, PlatformType::Normal));
    platforms.push_back(Platform(3700, 590, 430, 126, PlatformType::Normal));

    platforms.push_back(Platform(4240, 540, 300, 176, PlatformType::Normal));
    platforms.push_back(Platform(4610, 500, 300, 216, PlatformType::Normal));
    platforms.push_back(Platform(5000, 620, 480, 96, PlatformType::Normal));

    platforms.push_back(Platform(1040, 500, 150, 48, PlatformType::Breakable));
    platforms.push_back(Platform(1910, 500, 150, 48, PlatformType::Breakable));
    platforms.push_back(Platform(3420, 500, 160, 48, PlatformType::Breakable));

    // Temporary mode: enemies and old traps are kept in code, but not spawned on the level.
    // enemies.push_back(Enemy(...));
    // traps.push_back(Trap(..., TrapType::Fire));
    // traps.push_back(Trap(..., TrapType::MovingSaw));
    // traps.push_back(Trap(..., TrapType::FallingBlock));

    traps.push_back(Trap(940, 572, TrapType::Spikes, 144.f));
    traps.push_back(Trap(1710, 492, TrapType::Spikes, 96.f));
    traps.push_back(Trap(3920, 542, TrapType::Spikes, 144.f));
    traps.push_back(Trap(4750, 452, TrapType::Spikes, 96.f));

    diamonds.push_back(Diamond(250, 570));
    diamonds.push_back(Diamond(470, 570));
    diamonds.push_back(Diamond(910, 560));
    diamonds.push_back(Diamond(1090, 455));

    diamonds.push_back(Diamond(1320, 530));
    diamonds.push_back(Diamond(1660, 490));
    diamonds.push_back(Diamond(1810, 490));

    diamonds.push_back(Diamond(2180, 380));
    diamonds.push_back(Diamond(2320, 380));
    diamonds.push_back(Diamond(2440, 380));

    diamonds.push_back(Diamond(2700, 470));
    diamonds.push_back(Diamond(3120, 540));
    diamonds.push_back(Diamond(3810, 540));
    diamonds.push_back(Diamond(4020, 540));

    diamonds.push_back(Diamond(4310, 490));
    diamonds.push_back(Diamond(4680, 450));
    diamonds.push_back(Diamond(5080, 570));
    diamonds.push_back(Diamond(5220, 570));

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
                if (key->scancode == sf::Keyboard::Scancode::Space)
                {
                    player.jump();
                }

                if (key->scancode == sf::Keyboard::Scancode::Q)
                {
                    timeScale = 0.35f;
                }
            }

            if (const auto* key = event->getIf<sf::Event::KeyReleased>())
            {
                if (key->scancode == sf::Keyboard::Scancode::Q)
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
            else if (playerPos.x > levelWidth - 640.f)
                camera.setCenter({levelWidth - 640.f, 360.f});
            else
                camera.setCenter({playerPos.x, 360.f});

            for (auto& p : platforms)
            {
                p.update(scaledDt);

                if (!p.isBroken() && touch(player.getBounds(), p.getBounds()))
                {
                    bool landed = player.resolvePlatformCollision(p.getBounds());

                    if (landed && p.getType() == PlatformType::Breakable)
                        p.startBreaking();
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

            for (auto& d : diamonds)
            {
                d.update(dt);

                if (!d.isCollected() && touch(player.getBounds(), d.getBounds()))
                {
                    d.collect();
                    player.addDiamond();
                }
            }

            if (player.getHP() <= 0)
                gameOver = true;

            if (player.getPosition().x > 5250.f)
                victory = true;
        }

        window.clear(sf::Color(25, 20, 15));

        window.setView(camera);

        drawBackground(window, tiles, backgroundImage, levelWidth);

        for (auto& p : platforms)
            p.draw(window, tiles);

        for (auto& t : traps)
            t.draw(window, tiles);

        for (auto& e : enemies)
            e.draw(window);

        for (auto& d : diamonds)
            d.draw(window, tiles);

        player.draw(window);

        sf::RectangleShape treasureBase;
        treasureBase.setSize({70.f, 45.f});
        treasureBase.setFillColor(sf::Color(180, 100, 20));
        treasureBase.setPosition({5280.f, 575.f});
        window.draw(treasureBase);

        sf::RectangleShape treasureGold;
        treasureGold.setSize({58.f, 15.f});
        treasureGold.setFillColor(sf::Color(255, 215, 0));
        treasureGold.setPosition({5286.f, 560.f});
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
            sf::Text diamondText(font, "Diamonds: " + to_string(player.getDiamonds()), 28);
            diamondText.setFillColor(sf::Color::White);
            diamondText.setPosition({20.f, 60.f});
            window.draw(diamondText);

            sf::Text abilityText(font, "A/D move | Space jump | Shift dash | F hand attack | Q slow time", 20);
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
