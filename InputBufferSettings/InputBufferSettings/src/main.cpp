#include <Geode/Geode.hpp>
#include <Geode/modify/GJBaseGameLayer.hpp>
#include <Geode/modify/PlayerObject.hpp>
#include <Geode/modify/PlayLayer.hpp>

using namespace geode::prelude;

namespace {
	// ---- settings ---------------------------------------------------------
	struct Config {
		bool enabled = true;
		bool player2 = true;
		bool debug = false;
		int ground = 2;
		int wave = 2;
		int ship = 0;
		int ufo = 0;
		int swing = 0;
	};

	// Read fresh each call so changes in the settings menu apply immediately.
	Config loadConfig() {
		auto* m = Mod::get();
		Config c;
		c.enabled = m->getSettingValue<bool>("buffer-enabled");
		c.player2 = m->getSettingValue<bool>("apply-player2");
		c.debug   = m->getSettingValue<bool>("debug-log");
		c.ground  = static_cast<int>(m->getSettingValue<int64_t>("ground-ticks"));
		c.wave    = static_cast<int>(m->getSettingValue<int64_t>("wave-ticks"));
		c.ship    = static_cast<int>(m->getSettingValue<int64_t>("ship-ticks"));
		c.ufo     = static_cast<int>(m->getSettingValue<int64_t>("ufo-ticks"));
		c.swing   = static_cast<int>(m->getSettingValue<int64_t>("swing-ticks"));
		return c;
	}

	// ---- per-player state (index 0 = P1, 1 = P2) ----------------------------
	struct PlayerState {
		bool tracking = false;       // a buffered press is in progress
		bool releasePending = false; // the real release was swallowed and is waiting
		int ticks = 0;               // physics ticks since the press
		int required = 0;            // minimum hold length for this press
	};

	PlayerState g_state[2];
	bool g_bypass = false; // true while WE call handleButton, so the hook ignores it

	void resetStates() {
		g_state[0] = PlayerState{};
		g_state[1] = PlayerState{};
	}

	// Modes with no "ground" concept: the buffer always applies to these.
	bool isAirMode(PlayerObject* p) {
		return p->m_isDart || p->m_isShip || p->m_isBird || p->m_isSwing;
	}

	int ticksForMode(PlayerObject* p, Config const& c) {
		if (p->m_isDart)  return c.wave;
		if (p->m_isShip)  return c.ship;
		if (p->m_isBird)  return c.ufo;
		if (p->m_isSwing) return c.swing;
		return c.ground; // cube, ball, robot, spider
	}
}

class $modify(BufferGameLayer, GJBaseGameLayer) {
	void handleButton(bool down, int button, bool isPlayer1) {
		// Our own flush calls, and anything that isn't the jump button, pass straight through.
		if (g_bypass || button != 1) {
			GJBaseGameLayer::handleButton(down, button, isPlayer1);
			return;
		}

		auto& s = g_state[isPlayer1 ? 0 : 1];

		// A new press always cancels a swallowed release: emit that release first
		// so the engine sees a clean release -> press.
		if (down && s.releasePending) {
			s.releasePending = false;
			s.tracking = false;
			GJBaseGameLayer::handleButton(false, button, isPlayer1);
		}

		auto cfg = loadConfig();
		auto* player = isPlayer1 ? m_player1 : m_player2;

		if (!cfg.enabled || !player || (!isPlayer1 && !cfg.player2)) {
			s.tracking = false;
			GJBaseGameLayer::handleButton(down, button, isPlayer1);
			return;
		}

		if (down) {
			int n = ticksForMode(player, cfg);
			// Ground-type modes only need buffering when the press happens in the air;
			// on the ground the jump fires immediately anyway.
			bool eligible = n > 0 && (isAirMode(player) || !player->m_isOnGround);

			s.tracking = eligible;
			s.required = n;
			s.ticks = 0;
			s.releasePending = false;

			GJBaseGameLayer::handleButton(true, button, isPlayer1);
			return;
		}

		// Release: swallow it if the press hasn't been held long enough yet.
		if (s.tracking && s.ticks < s.required) {
			s.releasePending = true;
			return;
		}

		s.tracking = false;
		s.releasePending = false;
		GJBaseGameLayer::handleButton(false, button, isPlayer1);
	}
};

class $modify(BufferPlayer, PlayerObject) {
	void update(float dt) {
		PlayerObject::update(dt);

		auto* layer = m_gameLayer;
		if (!layer) return;

		int idx = -1;
		if (this == layer->m_player1) idx = 0;
		else if (this == layer->m_player2) idx = 1;
		if (idx < 0) return;

		auto& s = g_state[idx];
		if (!s.tracking) return; // idle: costs nothing

		if (s.ticks < s.required) ++s.ticks;

		if (s.releasePending && s.ticks >= s.required) {
			s.releasePending = false;
			s.tracking = false;

			g_bypass = true;
			layer->handleButton(false, 1, idx == 0);
			g_bypass = false;

			if (loadConfig().debug) {
				log::debug("[InputBuffer] delayed release applied for P{} after {} ticks", idx + 1, s.required);
			}
		}
	}
};

class $modify(BufferPlayLayer, PlayLayer) {
	void resetLevel() {
		resetStates();
		PlayLayer::resetLevel();
	}

	void onQuit() {
		resetStates();
		PlayLayer::onQuit();
	}
};
