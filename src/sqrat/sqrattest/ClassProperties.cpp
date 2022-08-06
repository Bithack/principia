//
// Copyright (c) 2009 Brandon Jones
//
// This software is provided 'as-is', without any express or implied
// warranty. In no event will the authors be held liable for any damages
// arising from the use of this software.
//
// Permission is granted to anyone to use this software for any purpose,
// including commercial applications, and to alter it and redistribute it
// freely, subject to the following restrictions:
//
//  1. The origin of this software must not be misrepresented; you must not
//  claim that you wrote the original software. If you use this software
//  in a product, an acknowledgment in the product documentation would be
//  appreciated but is not required.
//
//  2. Altered source versions must be plainly marked as such, and must not be
//  misrepresented as being the original software.
//
//  3. This notice may not be removed or altered from any source
//  distribution.
//

#include <gtest/gtest.h>
#include <sqrat.h>
#include "Fixture.h"

using namespace Sqrat;

struct Item {
    Item() {}
    Item(const Item& i) : name(i.name) {}
    string name;
};

class Player {
public:
    Player() : health(10) {};

    int Health() const {
        return health;
    }

    void SetHealth(const int& h) {
        health = h;
        if(health < 0) {
            health = 0;
        }
    }

    bool Dead() const {
        return (health == 0);
    }

    Item LeftHand() const {
        return leftHand;
    }

    void SetLeftHand(const Item& i) {
        leftHand = i;
    }

    Item RightHand() const {
        return rightHand;
    }

    void SetRightHand(const Item& i) {
        rightHand = i;
    }

private:
    int health;
    Item leftHand;
    Item rightHand;
};

TEST_F(SqratTest, ClassProperties) {
    DefaultVM::Set(vm);

    RootTable().Bind(_SC("Item"),
                     Class<Item>()
                     .Var(_SC("name"), &Item::name)
                    );

    RootTable().Bind(_SC("Player"),
                     Class<Player>()
                     // Properties
                     .Prop(_SC("health"), &Player::Health, &Player::SetHealth)
                     .Prop(_SC("dead"), &Player::Dead) // Read Only Property
                     .Prop(_SC("leftHand"), &Player::LeftHand, &Player::SetLeftHand)
                     .Prop(_SC("rightHand"), &Player::RightHand, &Player::SetRightHand)
                    );

    Script script;

    try {
        script.CompileString(_SC(" \
			p <- Player(); \
			gTest.EXPECT_INT_EQ(p.health, 10); \
			p.health = 5; \
			gTest.EXPECT_INT_EQ(p.health, 5); \
			p.health -= 3; \
			gTest.EXPECT_INT_EQ(p.health, 2); \
			p.health -= 3; \
			gTest.EXPECT_INT_EQ(p.health, 0); \
			gTest.EXPECT_TRUE(p.dead); \
			\
			item1 <- Item(); \
			item1.name = \"Sword\"; \
			p.rightHand = item1; \
			item2 <- Item(); \
			item2.name = \"Shield\"; \
			p.leftHand = item2; \
			gTest.EXPECT_STR_EQ(p.rightHand.name, \"Sword\"); \
			gTest.EXPECT_STR_EQ(p.leftHand.name, \"Shield\"); \
			"));
    } catch(Exception ex) {
        FAIL() << _SC("Compile Failed: ") << ex.Message();
    }

    try {
        script.Run();
    } catch(Exception ex) {
        FAIL() << _SC("Run Failed: ") << ex.Message();
    }
}