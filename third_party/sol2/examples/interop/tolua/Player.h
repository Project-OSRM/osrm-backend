#ifndef PLAYER_H
#define PLAYER_H

class Player {
private:
	int m_health;

public:
	Player()
	: m_health(0) {
	}
	void setHealth(int health) {
		m_health = health;
	}
	int getHealth() const {
		return m_health;
	}
};

#endif // PLAYER_H
