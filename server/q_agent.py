"""
q_agent.py — Deep Q-Network agent for the RL car simulation.

State  : 6 continuous values [dist_l, dist_fl, dist_f, dist_fr, dist_r, velocity]
Actions: 4 discrete — 0=Left, 1=Right, 2=Accelerate, 3=Brake
"""

import torch
import torch.nn as nn
import torch.nn.functional as F
import random
import os
from collections import deque

NUM_ACTIONS = 4
ACTION_NAMES = ["Left", "Right", "Accelerate", "Brake"]

class ReplayBuffer:
    def __init__(self, capacity=10000):
        self.buffer = deque(maxlen=capacity)

    def push(self, state, action, reward, next_state, done):
        self.buffer.append((state, action, reward, next_state, done))

    def sample(self, batch_size):
        batch = random.sample(self.buffer, batch_size)
        states, actions, rewards, next_states, dones = zip(*batch)
        return list(states), list(actions), list(rewards), list(next_states), list(dones)

    def __len__(self):
        return len(self.buffer)


class DQN(nn.Module):
    def __init__(self, input_dim=6, output_dim=4):
        super(DQN, self).__init__()
        self.fc1 = nn.Linear(input_dim, 64)
        self.fc2 = nn.Linear(64, 64)
        self.fc3 = nn.Linear(64, output_dim)

    def forward(self, x):
        x = F.relu(self.fc1(x))
        x = F.relu(self.fc2(x))
        return self.fc3(x)

class DQNAgent:
    def __init__(self, learning_rate=1e-3, gamma=0.99, epsilon=1.0, epsilon_decay=0.97, epsilon_min=0.05):
        # Use CPU for small network to avoid GPU transfer overhead, unless specifically wanting CUDA.
        # For a 64-node MLP, CPU is often faster than CUDA due to batch transfer overhead during inference.
        self.device = torch.device("cpu")
        self.gamma = gamma
        self.epsilon = epsilon
        self.epsilon_decay = epsilon_decay
        self.epsilon_min = epsilon_min
        
        self.policy_net = DQN().to(self.device)
        self.target_net = DQN().to(self.device)
        self.target_net.load_state_dict(self.policy_net.state_dict())
        self.target_net.eval()
        
        self.optimizer = torch.optim.Adam(self.policy_net.parameters(), lr=learning_rate)
        self.criterion = nn.MSELoss()
        
        self.memory = ReplayBuffer(capacity=50000)
        self.batch_size = 64
        self.update_steps = 0

    def choose_action(self, state: list, epsilon: float = None) -> int:
        """ε-greedy action selection. Pass epsilon=0.0 for pure exploitation."""
        eps = epsilon if epsilon is not None else self.epsilon
        if random.random() < eps:
            return random.randint(0, NUM_ACTIONS - 1)
        
        with torch.no_grad():
            state_tensor = torch.FloatTensor(state).unsqueeze(0).to(self.device)
            q_values = self.policy_net(state_tensor)
            return q_values.argmax(dim=1).item()

    def push(self, state, action, reward, next_state, done):
        self.memory.push(state, action, reward, next_state, done)
        
    def train_step(self):
        if len(self.memory) < self.batch_size:
            return 0.0

        states, actions, rewards, next_states, dones = self.memory.sample(self.batch_size)

        states_tensor = torch.FloatTensor(states).to(self.device)
        actions_tensor = torch.LongTensor(actions).to(self.device)
        rewards_tensor = torch.FloatTensor(rewards).to(self.device)
        next_states_tensor = torch.FloatTensor(next_states).to(self.device)
        dones_tensor = torch.FloatTensor(dones).to(self.device)

        q_values = self.policy_net(states_tensor)
        current_q = q_values.gather(1, actions_tensor.unsqueeze(1)).squeeze(1)

        with torch.no_grad():
            next_q = self.target_net(next_states_tensor).max(1)[0]
            target = rewards_tensor + self.gamma * next_q * (1 - dones_tensor)

        loss = self.criterion(current_q, target)

        self.optimizer.zero_grad()
        loss.backward()
        self.optimizer.step()

        self.update_steps += 1
        # Update target network much less frequently to stabilize Q-value targets
        if self.update_steps % 1000 == 0:
            self.update_target_network()
            
        return loss.item()

    def decay_epsilon(self):
        self.epsilon = max(self.epsilon_min, self.epsilon * self.epsilon_decay)
            
    def update_target_network(self):
        self.target_net.load_state_dict(self.policy_net.state_dict())

    def save(self, path: str) -> None:
        torch.save({
            'policy_net': self.policy_net.state_dict(),
            'epsilon': self.epsilon
        }, path)
        print(f"[DQNAgent] Saved model to {path}")

    def load(self, path: str) -> None:
        if not os.path.exists(path):
            return
        checkpoint = torch.load(path, map_location=self.device)
        self.policy_net.load_state_dict(checkpoint['policy_net'])
        self.target_net.load_state_dict(self.policy_net.state_dict())
        # We purposely do not load epsilon here, so it always starts at 1.0
        # if 'epsilon' in checkpoint:
        #     self.epsilon = checkpoint['epsilon']
        print(f"[DQNAgent] Loaded model from {path}. Epsilon reset to {self.epsilon:.2f}")
