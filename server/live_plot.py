import pandas as pd
import matplotlib.pyplot as plt
import matplotlib.animation as animation
import os

METRICS_FILE = os.path.join(os.path.dirname(os.path.abspath(__file__)), "metrics.csv")

# Ensure file exists at started (even if empty) to prevent crashing
if not os.path.exists(METRICS_FILE):
    print(f"Waiting for {METRICS_FILE} to be created by the server...")

# Setup Figure
fig, (ax1, ax2) = plt.subplots(2, 1, figsize=(5,7))
fig.suptitle("RL Agent Real-Time Metrics", fontsize=16)

def animate(i):
    if not os.path.exists(METRICS_FILE):
        return
    
    try:
        df = pd.read_csv(METRICS_FILE)
    except Exception:
        return
    
    if df.empty or "Episode" not in df.columns:
        return

    episodes = df["Episode"].values
    rewards = df["Reward"].values
    epsilons = df["Epsilon"].values
    
    # Update Plot 1 (Rewards)
    ax1.clear()
    ax1.plot(episodes, rewards, color='blue', label='Total Reward', linewidth=2)
    ax1.set_xlabel("Episode")
    ax1.set_ylabel("Total Reward")
    ax1.set_title("Episodes vs Total Reward")
    ax1.grid(True)
    ax1.legend(loc="upper left")
    
    # Update Plot 2 (Epsilon only)
    ax2.clear()
    ax2.plot(episodes, epsilons, color='green', label='Epsilon', linestyle='solid', linewidth=2)
    ax2.set_xlabel("Episode")
    ax2.set_ylabel("Epsilon (Exploration Rate)", color='green')
    ax2.tick_params(axis='y', labelcolor='green')
    ax2.set_ylim([0, 1.05])
    ax2.legend(loc="upper right")
    ax2.set_title("Training Progress: Epsilon")
    ax2.grid(True)
    
animate(0) # Plot what is available initially
# Use cache_frame_data=False if Matplotlib version allows
try:
    ani = animation.FuncAnimation(fig, animate, interval=1000, cache_frame_data=False)
except TypeError:
    ani = animation.FuncAnimation(fig, animate, interval=1000)

plt.tight_layout()
plt.subplots_adjust(top=0.92) # Leave space for suptitle
plt.show()
