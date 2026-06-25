import express from 'express';
import cors from 'cors';
import net from 'net';
import path from 'path';
import { fileURLToPath } from 'url';

const __filename = fileURLToPath(import.meta.url);
const __dirname = path.dirname(__filename);

const app = express();
const PORT = 3001; // Runs on port 3001 so both can be active at the same time
const DB_PORT = 2006;
const DB_HOST = '127.0.0.1';

app.use(cors());
app.use(express.json());
app.use(express.static(path.join(__dirname, 'public')));

// Create a single persistent connection
const dbClient = new net.Socket();
let isConnected = false;

// We use a queue to match asynchronous responses to their incoming HTTP requests
const requestQueue = [];
let incomingBuffer = '';

dbClient.connect(DB_PORT, DB_HOST, () => {
    console.log(`[Persistent Server] Connected to RAMpage DB on port ${DB_PORT}`);
    isConnected = true;
});

dbClient.on('data', (data) => {
    incomingBuffer += data.toString();
    
    // Process full lines (just like our C++ server does)
    let newlineIdx;
    while ((newlineIdx = incomingBuffer.indexOf('\n')) !== -1) {
        const line = incomingBuffer.substring(0, newlineIdx);
        incomingBuffer = incomingBuffer.substring(newlineIdx + 1);
        
        // Resolve the oldest pending request in the queue
        if (requestQueue.length > 0) {
            const { res } = requestQueue.shift();
            res.json({ response: line.trim() });
        }
    }
});

dbClient.on('error', (err) => {
    console.error('[Persistent Server] Socket error:', err.message);
    isConnected = false;
});

dbClient.on('close', () => {
    console.log('[Persistent Server] Connection closed by RAMpage');
    isConnected = false;
});

app.post('/execute', (req, res) => {
    const command = req.body.command;
    if (!command) {
        return res.status(400).json({ error: 'Command is required' });
    }

    if (!isConnected) {
        return res.status(500).json({ error: 'Database is disconnected. Is RAMpage running?' });
    }

    // Queue the Express response object so we can reply when the data comes back
    requestQueue.push({ res });

    // Send the command through the open pipeline
    dbClient.write(command + '\n');
});

app.listen(PORT, () => {
    console.log(`[Persistent Server] Listening on http://localhost:${PORT}`);
    console.log(`[Persistent Server] Uses ONE shared TCP connection for all HTTP requests.`);
});
