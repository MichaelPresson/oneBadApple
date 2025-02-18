# One Bad Apple

## Overview
One Bad Apple is a process synchronization and inter-process communication (IPC) project designed to simulate a **circular communication system** where `k` processes form a ring. Each process can only send or receive messages when it possesses the **apple**, which acts as a synchronization token. The system ensures that messages are passed around efficiently while maintaining proper synchronization.

## Features
- **Ring-based communication:** Each process communicates with its direct neighbor via pipes.
- **Token-based synchronization:** A process can only send/receive when it has the **apple**.
- **Message passing:** Users can send messages to a specific node in the ring.
- **Graceful termination:** The system properly handles `Ctrl+C` to shut down all processes.
- **Verbose logging:** Clear diagnostic messages display process actions and message flow.

## Installation
To set up the project on your local system:

```bash
# Clone the repository
git clone https://github.com/MichaelPresson/oneBadApple.git

# Navigate to the project directory
cd oneBadApple

# Compile the program
gcc -o oneBadApple oneBadApple.c
```

## Usage
Run the program and follow the prompts:

```bash
./oneBadApple
```

### How It Works
1. The program prompts the user for the number of processes (`k`).
2. The parent process (Node 0) creates `k-1` child processes, forming a ring.
3. The user enters a message and specifies a destination node.
4. The **apple** (token) controls message passing in the system.
5. Messages are forwarded until they reach their intended recipient.
6. The system waits for the apple to return to the parent before prompting for the next message.
7. Press `Ctrl+C` to gracefully terminate all processes.

## Example Run
```
Enter number of processes: 5
Creating 5 processes...
Parent (Node 0) PID: 1234
Child Process 1 created with PID: 1235
Child Process 2 created with PID: 1236
Child Process 3 created with PID: 1237
Child Process 4 created with PID: 1238

Enter message: Hello
Enter destination node: 3
[Node 0] Sending 'Hello' to Node 3
[Node 1] Forwarding message to Node 2
[Node 2] Forwarding message to Node 3
[Node 3] Received message: 'Hello'
```

## Contributing
Contributions are welcome! If you'd like to contribute:
1. **Fork** the repository.
2. Create a **feature branch** (`git checkout -b feature-name`).
3. Commit your changes (`git commit -m "Added feature XYZ"`).
4. Push to your branch (`git push origin feature-name`).
5. Submit a **Pull Request**.

## License
This project is licensed under the MIT License. See `LICENSE` for details.

## Contact
For any questions or issues, feel free to reach out:
- **GitHub Issues:** [Open an issue](https://github.com/MichaelPresson/oneBadApple/issues)
- **Email:** [Your Email Here]

---
Enjoy coding! 🚀

