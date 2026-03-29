import socket
import csv

HOST = '127.0.0.1'
METRICS_PATH = "server/metrics.csv"

PORT = 5000
def main():
    server = socket.socket(socket.AF_INET, socket.SOCK_STREAM)
    server.bind((HOST, PORT))
    server.listen()

    print("Server listening...")

    conn, addr = server.accept()
    print(f"Connected to {addr}")

    while True:
        data = conn.recv(1024).decode()
        if not data:
            break

        print("Client:", data)

        # Dummy AI logic
        response = data.upper()

        conn.send(response.encode())

    conn.close()

ab = csv.reader(open(METRICS_PATH, "r"))
ab = list(ab)
episodes = ab[-1][0]
print(episodes)
if __name__ == "__main__":
    print("okay")