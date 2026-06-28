import React from 'react';
import { Card, CardHeader, CardTitle, CardDescription, CardContent } from '../components/ui/card';
import { Terminal, Github, Code, Play } from 'lucide-react';

function GettingStarted() {
    return (
        <div className="container mx-auto px-4 py-12 flex-1">
            <div className="max-w-4xl mx-auto">
                <div className="mb-12">
                    <h1 className="text-4xl md:text-5xl font-extrabold tracking-tight mb-4">
                        Getting Started
                    </h1>
                    <p className="text-xl text-muted-foreground">
                        Get RAMpage running locally in seconds using Docker, and connect with your favorite Redis tools.
                    </p>
                </div>

                <div className="space-y-12">
                    {/* Step 1: Docker */}
                    <section>
                        <h2 className="text-2xl font-bold mb-6 flex items-center gap-2">
                            <span className="flex items-center justify-center bg-primary text-primary-foreground rounded-full w-8 h-8 text-sm">1</span>
                            Clone & Run
                        </h2>
                        <Card className="border-primary/20 shadow-sm">
                            <CardHeader>
                                <CardTitle className="flex items-center gap-2"><Github className="w-5 h-5" /> Use Docker to instantly start the server</CardTitle>
                                <CardDescription>This is the recommended way to run RAMpage without dealing with C++ build dependencies.</CardDescription>
                            </CardHeader>
                            <CardContent className="space-y-4">
                                <pre className="bg-muted/50 p-4 rounded-md font-mono text-sm overflow-x-auto border text-muted-foreground">
                                    <code>
                                        <span className="text-primary">git clone</span> https://github.com/prana-w/rampage.git{"\n"}
                                        <span className="text-primary">cd</span> rampage{"\n"}
                                        <span className="text-primary">docker build</span> -t rampage-server .{"\n"}
                                        <span className="text-primary">docker run</span> -p 2006:2006 --name my-rampage -d rampage-server
                                    </code>
                                </pre>
                                <p className="text-sm text-muted-foreground">
                                    RAMpage runs on port <code>2006</code> by default to avoid conflicting with any existing Redis instances running on <code>6379</code>.
                                </p>
                            </CardContent>
                        </Card>
                    </section>

                    {/* Step 2: Connect */}
                    <section>
                        <h2 className="text-2xl font-bold mb-6 flex items-center gap-2">
                            <span className="flex items-center justify-center bg-primary text-primary-foreground rounded-full w-8 h-8 text-sm">2</span>
                            Connect & Interact
                        </h2>
                        <Card className="border-primary/20 shadow-sm">
                            <CardHeader>
                                <CardTitle className="flex items-center gap-2"><Terminal className="w-5 h-5" /> Use the official Redis CLI or standard SDKs</CardTitle>
                                <CardDescription>Because RAMpage speaks native RESP, you don't need any custom SDKs.</CardDescription>
                            </CardHeader>
                            <CardContent className="space-y-6">
                                <div>
                                    <h3 className="font-semibold mb-2">Via redis-cli</h3>
                                    <pre className="bg-muted/50 p-4 rounded-md font-mono text-sm overflow-x-auto border text-muted-foreground">
                                        <code>
                                            redis-cli -p 2006
                                        </code>
                                    </pre>
                                </div>
                                <div>
                                    <h3 className="font-semibold mb-2">Via Node.js (Official redis package)</h3>
                                    <pre className="bg-muted/50 p-4 rounded-md font-mono text-sm overflow-x-auto border text-muted-foreground">
                                        <code>
                                            import &#123; createClient &#125; from 'redis';{"\n\n"}
                                            const client = createClient(&#123; url: 'redis://127.0.0.1:2006' &#125;);{"\n"}
                                            await client.connect();{"\n\n"}
                                            await client.set('hello', 'world');{"\n"}
                                            const val = await client.get('hello');{"\n"}
                                            console.log(val); // prints 'world'
                                        </code>
                                    </pre>
                                </div>
                                <div className="p-4 rounded-lg bg-destructive/10 border border-destructive/20 text-destructive text-sm font-medium">
                                    <strong>Note:</strong> RAMpage is actively under development. While it perfectly supports core String and List operations, it does not yet support every single command in the massive Redis catalog. If you send an unsupported command, RAMpage will safely return an <code>ERR: unknown command</code> response.
                                </div>
                            </CardContent>
                        </Card>
                    </section>

                    {/* Step 3: Benchmarking */}
                    <section>
                        <h2 className="text-2xl font-bold mb-6 flex items-center gap-2">
                            <span className="flex items-center justify-center bg-primary text-primary-foreground rounded-full w-8 h-8 text-sm">3</span>
                            Benchmark It Yourself
                        </h2>
                        <Card className="border-primary/20 shadow-sm">
                            <CardHeader>
                                <CardTitle className="flex items-center gap-2"><Play className="w-5 h-5" /> Test the raw throughput and latency</CardTitle>
                                <CardDescription>Run this command on your machine to see how RAMpage handles heavy load.</CardDescription>
                            </CardHeader>
                            <CardContent className="space-y-4">
                                <pre className="bg-muted/50 p-4 rounded-md font-mono text-sm overflow-x-auto border text-muted-foreground">
                                    <code>
                                        redis-benchmark -p 2006 -t ping,set,get,lpush,lpop -n 100000 -q
                                    </code>
                                </pre>
                            </CardContent>
                        </Card>
                    </section>
                </div>
            </div>
        </div>
    );
}

export default GettingStarted;
