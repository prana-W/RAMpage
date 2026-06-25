import express from 'express';
import cors from 'cors';
import net from 'net';
import path from 'path';
import { fileURLToPath } from 'url';

const __filename = fileURLToPath(import.meta.url);
const __dirname = path.dirname(__filename);

const app = express();
const PORT = 3000;
const DB_PORT = 2006;
const DB_HOST = '127.0.0.1';

app.use(cors());
app.use(express.json());
app.use(express.static(path.join(__dirname, 'public')));

app.post('/execute', (req, res) => {
    const command = req.body.command;
    if (!command) {
        return res.status(400).json({ error: 'Command is required' });
    }

    const client = new net.Socket();
    let responseData = '';

    client.connect(DB_PORT, DB_HOST, () => {
        // Send the command and immediately flush (with newline)
        client.write(command + '\n');
    });

    client.on('data', (data) => {
        responseData += data.toString();
        // Since we expect a single atomic response per connection, we can close it
        client.destroy(); 
    });

    client.on('close', () => {
        res.json({ response: responseData.trim() });
    });

    client.on('error', (err) => {
        console.error('[New-Conn] Socket error:', err.message);
        res.status(500).json({ error: err.message });
    });
});

app.listen(PORT, () => {
    console.log(`[New-Conn Server] Listening on http://localhost:${PORT}`);
    console.log(`[New-Conn Server] Every HTTP request opens a brand new TCP connection to RAMpage.`);
});
