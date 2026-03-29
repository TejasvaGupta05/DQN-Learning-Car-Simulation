"""
inference_server.py — TCP server acting as the 'brain' using the PyTorch DQN.

Listens on 127.0.0.1:5000.
"""

import socket
import json
import os
import sys
import threading
import csv

sys.path.insert(0, os.path.dirname(os.path.abspath(__file__)))
from q_agent import DQNAgent, ACTION_NAMES

HOST       = "127.0.0.1"
PORT       = 5000
MODEL_PATH = "server/dqn.pth"
METRICS_PATH = "server/metrics.csv"

def handle_client(conn: socket.socket, addr, agent: DQNAgent) -> None:
    print(f"[server] Connected: {addr}")
    buffer = ""
    last_state = None
    last_action = None
    episodes = 0
    try:
        with open(METRICS_PATH, "r") as f:
            rows = list(csv.reader(f))
            if len(rows) > 1:
                episodes = int(rows[-1][0])
    except Exception:
        pass
    episode_reward = 0.0
    episode_losses = []

    try:
        while True:
            chunk = conn.recv(1024)
            if not chunk:
                break
            buffer += chunk.decode("utf-8", errors="replace")

            while "\n" in buffer:
                line, buffer = buffer.split("\n", 1)
                line = line.strip()
                if not line:
                    continue

                try:
                    msg        = json.loads(line)
                    curr_state = msg["state"]
                    reward     = msg.get("reward", 0.0)
                    done       = msg.get("done", False)

                    episode_reward += reward

                    if last_state is not None:
                        agent.push(last_state, last_action, reward, curr_state, done)
                        loss = agent.train_step()
                        if loss is not None and loss > 0.0:
                            episode_losses.append(loss)

                    if done:
                        episodes += 1
                        agent.decay_epsilon()
                        avg_loss = sum(episode_losses) / len(episode_losses) if episode_losses else 0.0
                        print(f"[server] Episode {episodes} completed. Reward: {episode_reward:.2f}, Epsilon: {agent.epsilon:.3f}, Loss: {avg_loss:.4f}")
                        
                        try:
                            with open(METRICS_PATH, "a", newline="") as f:
                                writer = csv.writer(f)
                                writer.writerow([episodes, episode_reward, agent.epsilon, avg_loss])
                        except Exception as e:
                            print(f"[server] Failed to write metrics: {e}")

                        agent.save(MODEL_PATH)
                        episode_reward = 0.0
                        episode_losses = []

                    action = agent.choose_action(curr_state)
                    last_state  = curr_state
                    last_action = action

                    response = json.dumps({"action": action}) + "\n"
                    conn.sendall(response.encode("utf-8"))

                except (json.JSONDecodeError, KeyError, IndexError) as e:
                    print(f"[server] JSON/Parse error: {e}")

    except ConnectionResetError:
        pass
    except Exception as e:
        print(f"[server] Exception: {e}")
    finally:
        conn.close()
        print(f"[server] Disconnected: {addr}")


def main() -> None:
    if not os.path.exists(METRICS_PATH):
        try:
            with open(METRICS_PATH, "w", newline="") as f:
                writer = csv.writer(f)
                writer.writerow(["Episode", "Reward", "Epsilon", "Loss"])
        except Exception as e:
            print(f"[server] Warning: Could not create metrics.csv: {e}")

    agent = DQNAgent()
    if os.path.exists(MODEL_PATH):
        agent.load(MODEL_PATH)
    else:
        print(f"[server] WARNING: No model at '{MODEL_PATH}'. Using random policy.")

    srv = socket.socket(socket.AF_INET, socket.SOCK_STREAM)
    srv.setsockopt(socket.SOL_SOCKET, socket.SO_REUSEADDR, 1)
    srv.bind((HOST, PORT))
    srv.listen(5)
    print(f"[server] Listening on {HOST}:{PORT} (Epsilon={agent.epsilon:.3f})")

    try:
        while True:
            conn, addr = srv.accept()
            t = threading.Thread(
                target=handle_client, args=(conn, addr, agent), daemon=True
            )
            t.start()
    except KeyboardInterrupt:
        print("\n[server] Shutting down.")
    finally:
        srv.close()


if __name__ == "__main__":
    main()
