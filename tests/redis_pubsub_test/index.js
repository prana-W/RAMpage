import { createClient } from 'redis';

// Parse command line arguments for port
const args = process.argv.slice(2);
const port = args.length > 0 ? parseInt(args[0], 10) : 2006;

if (isNaN(port)) {
    console.error("Invalid port specified.");
    process.exit(1);
}

// --- STRESS TEST PARAMETERS ---
// These parameters will create a massive fan-out scenario.
// 200 subscribers * 10,000 publishes = 2,000,000 messages pushed.
const NUM_SUBSCRIBERS = 200;
const NUM_PUBLISHES = 10000;
const CHANNEL_NAME = 'stress-channel-main';
const PATTERN_NAME = 'stress-channel-*';

console.log(`\n🔥 STARTING PUBSUB STRESS TEST ON PORT ${port} 🔥`);
console.log(`Subscribers: ${NUM_SUBSCRIBERS}`);
console.log(`Publishes:   ${NUM_PUBLISHES}`);
console.log(`Total Expected Pushes: ${(NUM_SUBSCRIBERS * NUM_PUBLISHES).toLocaleString()}`);
console.log('----------------------------------------------------');

async function run() {
    let exactMessagesReceived = 0;
    let patternMessagesReceived = 0;
    const subscribers = [];
    
    // Create the publisher connection
    const publisher = createClient({ url: `redis://127.0.0.1:${port}` });
    
    // Connect publisher and ping to ensure server is alive
    try {
        await publisher.connect();
        await publisher.ping();
    } catch (e) {
        console.error(`❌ Failed to connect to server on port ${port}. Is it running? Error:`, e);
        process.exit(1);
    }

    console.log(`[1] Booting up ${NUM_SUBSCRIBERS} concurrent subscribers...`);
    
    // Connect all subscribers concurrently for speed
    const connectPromises = [];
    for (let i = 0; i < NUM_SUBSCRIBERS; i++) {
        const sub = createClient({ url: `redis://127.0.0.1:${port}` });
        
        // Even clients do EXACT subscribe, odd clients do PATTERN subscribe
        const isPattern = i % 2 !== 0;
        
        const p = sub.connect().then(() => {
            if (isPattern) {
                return sub.pSubscribe(PATTERN_NAME, (message, channel) => {
                    patternMessagesReceived++;
                });
            } else {
                return sub.subscribe(CHANNEL_NAME, (message) => {
                    exactMessagesReceived++;
                });
            }
        });
        
        connectPromises.push(p);
        subscribers.push(sub);
    }

    await Promise.all(connectPromises);
    console.log(`[2] All ${NUM_SUBSCRIBERS} subscribers connected and listening.`);
    console.log(`    -> ${NUM_SUBSCRIBERS / 2} on EXACT channel '${CHANNEL_NAME}'`);
    console.log(`    -> ${NUM_SUBSCRIBERS / 2} on PATTERN '${PATTERN_NAME}'`);
    console.log(`[3] Hammering server with ${NUM_PUBLISHES} back-to-back PUBLISH commands...`);
    
    const startTime = Date.now();
    const publishPromises = [];

    // Fire off all publish commands concurrently in a massive pipeline
    for (let i = 0; i < NUM_PUBLISHES; i++) {
        // Publish to the exact channel (which also matches the pattern!)
        // This means EVERY publish hits ALL 200 subscribers.
        publishPromises.push(publisher.publish(CHANNEL_NAME, `payload-${i}`));
    }

    // Wait for the publisher to finish pushing all commands to the OS socket buffer
    await Promise.all(publishPromises);
    const publishTimeMs = Date.now() - startTime;
    console.log(`[4] Finished issuing all PUBLISH commands in ${publishTimeMs} ms.`);
    console.log(`[5] Waiting for all messages to arrive at subscribers...`);

    const TARGET_TOTAL = NUM_SUBSCRIBERS * NUM_PUBLISHES;
    let prevTotal = 0;
    
    // Wait until all messages are received or we time out
    const timeout = setTimeout(() => {
        console.error(`\n❌ TIMEOUT: Test stalled. Received ${exactMessagesReceived + patternMessagesReceived} / ${TARGET_TOTAL}`);
        process.exit(1);
    }, 15000); // 15 seconds max

    while ((exactMessagesReceived + patternMessagesReceived) < TARGET_TOTAL) {
        await new Promise(r => setTimeout(r, 50));
        const currentTotal = exactMessagesReceived + patternMessagesReceived;
        if (currentTotal > prevTotal && currentTotal % 200000 === 0) {
            console.log(`    ...received ${currentTotal.toLocaleString()} messages so far...`);
            prevTotal = currentTotal;
        }
    }
    clearTimeout(timeout);

    const totalTimeMs = Date.now() - startTime;
    const totalReceived = exactMessagesReceived + patternMessagesReceived;

    console.log(`\n✅ TEST COMPLETE!`);
    console.log(`====================================================`);
    console.log(`Total Time:          ${totalTimeMs} ms`);
    console.log(`Exact Received:      ${exactMessagesReceived.toLocaleString()}`);
    console.log(`Pattern Received:    ${patternMessagesReceived.toLocaleString()}`);
    console.log(`Total Messages Pushed: ${totalReceived.toLocaleString()}`);
    
    const throughput = Math.round((totalReceived / totalTimeMs) * 1000);
    console.log(`\n🚀 FAN-OUT THROUGHPUT: ${throughput.toLocaleString()} messages/sec`);
    console.log(`====================================================\n`);

    // Teardown
    await publisher.quit();
    for (const sub of subscribers) {
        await sub.quit();
    }
    process.exit(0);
}

run();
