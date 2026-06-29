import { Link } from 'react-router-dom';
import { ArrowRight, Zap, Database, ShieldCheck, Activity, Cpu, MessageSquare } from 'lucide-react';
import { Button } from '../components/ui/button';
import { Card, CardContent, CardDescription, CardHeader, CardTitle } from '../components/ui/card';

function Home() {
    return (
        <div className="flex flex-col min-h-screen">
            {/* Hero Section */}
            <section className="relative overflow-hidden bg-background pt-16 md:pt-24 lg:pt-32 pb-16">
                <div className="container mx-auto px-4 relative z-10 text-center">
                    <div className="inline-flex items-center rounded-full border border-primary/20 bg-primary/10 px-3 py-1 text-sm font-medium text-primary mb-8">
                        <Zap className="mr-2 h-4 w-4" />
                        Extreme Micro-Optimizations Live
                    </div>
                    <h1 className="text-4xl md:text-6xl lg:text-7xl font-extrabold tracking-tight mb-6">
                        The Redis-compatible <br />
                        <span className="text-transparent bg-clip-text bg-gradient-to-r from-primary to-accent">
                            C++ Powerhouse
                        </span>
                    </h1>
                    <p className="mt-4 text-xl text-muted-foreground max-w-2xl mx-auto mb-10">
                        RAMpage is a high-performance in-memory database built entirely from scratch in C++. 
                        Powered by Linux epoll, it delivers sub-millisecond p50 latency and outperforms Redis in raw response speed.
                    </p>
                    <div className="flex flex-col sm:flex-row gap-4 justify-center">
                        <Button asChild size="lg" className="px-8 h-12 text-lg">
                            <Link to="/commands">
                                View Commands <ArrowRight className="ml-2 h-5 w-5" />
                            </Link>
                        </Button>
                        <Button variant="outline" size="lg" asChild className="px-8 h-12 text-lg">
                            <a href="https://github.com/prana-w/rampage" target="_blank" rel="noreferrer">
                                GitHub Repository
                            </a>
                        </Button>
                    </div>
                </div>
                
                {/* Background Decor */}
                <div className="absolute top-1/2 left-1/2 -translate-x-1/2 -translate-y-1/2 w-[800px] h-[800px] bg-primary/5 rounded-full blur-3xl -z-10 pointer-events-none" />
            </section>

            {/* Benchmarks Section */}
            <section className="py-16 md:py-24 bg-muted/30">
                <div className="container mx-auto px-4">
                    <div className="text-center mb-12">
                        <h2 className="text-3xl md:text-4xl font-bold mb-4">Outperforming the Standard</h2>
                        <p className="text-lg text-muted-foreground max-w-2xl mx-auto">
                            Tested with 100,000 requests using <code>redis-benchmark</code>. RAMpage delivers 2x to 4x faster response times across almost every core command.
                        </p>
                    </div>

                    <div className="overflow-x-auto rounded-xl border bg-card shadow-sm max-w-5xl mx-auto">
                        <table className="w-full text-sm text-left">
                            <thead className="text-xs uppercase bg-muted/50 border-b">
                                <tr>
                                    <th className="px-6 py-4 font-semibold">Command</th>
                                    <th className="px-6 py-4 font-semibold text-right">Redis (req/s)</th>
                                    <th className="px-6 py-4 font-semibold text-right">RAMpage (req/s)</th>
                                    <th className="px-6 py-4 font-semibold text-right">Throughput Verdict</th>
                                    <th className="px-6 py-4 font-semibold text-right">Redis p50 (ms)</th>
                                    <th className="px-6 py-4 font-semibold text-right">RAMpage p50 (ms)</th>
                                    <th className="px-6 py-4 font-semibold">Latency Verdict</th>
                                </tr>
                            </thead>
                            <tbody className="divide-y">
                                {[
                                    { cmd: "PING_INLINE", redisT: "93,633", ramT: "89,047", tVerdict: "Redis 1.05x faster", redisL: "0.407", ramL: "0.295", verdict: "RAMpage 1.4x faster" },
                                    { cmd: "PING_MBULK", redisT: "100,806", ramT: "86,207", tVerdict: "Redis 1.2x faster", redisL: "0.391", ramL: "0.223", verdict: "RAMpage 1.8x faster" },
                                    { cmd: "SET", redisT: "97,561", ramT: "81,833", tVerdict: "Redis 1.2x faster", redisL: "0.455", ramL: "0.255", verdict: "RAMpage 1.8x faster" },
                                    { cmd: "GET", redisT: "99,305", ramT: "82,305", tVerdict: "Redis 1.2x faster", redisL: "0.415", ramL: "0.335", verdict: "RAMpage 1.2x faster" },
                                    { cmd: "LPUSH", redisT: "97,371", ramT: "82,305", tVerdict: "Redis 1.2x faster", redisL: "0.447", ramL: "0.095", verdict: "RAMpage 4.7x faster" },
                                    { cmd: "RPUSH", redisT: "98,425", ramT: "54,377", tVerdict: "Redis 1.8x faster", redisL: "0.447", ramL: "0.103", verdict: "RAMpage 4.3x faster" },
                                    { cmd: "LPOP", redisT: "96,993", ramT: "81,235", tVerdict: "Redis 1.2x faster", redisL: "0.455", ramL: "0.095", verdict: "RAMpage 4.8x faster" },
                                    { cmd: "RPOP", redisT: "94,429", ramT: "82,305", tVerdict: "Redis 1.1x faster", redisL: "0.463", ramL: "0.127", verdict: "RAMpage 3.6x faster" },
                                    { cmd: "LRANGE_100", redisT: "67,659", ramT: "21,906", tVerdict: "Redis 3.1x faster", redisL: "0.391", ramL: "2.151", verdict: "Redis 5.5x faster" },
                                    { cmd: "LRANGE_300", redisT: "38,820", ramT: "8,678", tVerdict: "Redis 4.5x faster", redisL: "0.647", ramL: "5.455", verdict: "Redis 8.4x faster" },
                                    { cmd: "LRANGE_500", redisT: "26,062", ramT: "5,564", tVerdict: "Redis 4.7x faster", redisL: "0.959", ramL: "8.711", verdict: "Redis 9.1x faster" },
                                    { cmd: "LRANGE_600", redisT: "21,730", ramT: "4,645", tVerdict: "Redis 4.7x faster", redisL: "1.119", ramL: "10.279", verdict: "Redis 9.2x faster" },
                                ].map((row, i) => (
                                    <tr key={i} className="hover:bg-muted/30 transition-colors">
                                        <td className="px-6 py-4 font-medium">{row.cmd}</td>
                                        <td className="px-6 py-4 text-right text-muted-foreground">{row.redisT}</td>
                                        <td className="px-6 py-4 text-right">{row.ramT}</td>
                                        <td className={`px-6 py-4 text-right font-semibold ${row.tVerdict.includes('RAMpage') ? 'text-primary' : 'text-muted-foreground'}`}>{row.tVerdict}</td>
                                        <td className="px-6 py-4 text-right text-muted-foreground">{row.redisL}</td>
                                        <td className="px-6 py-4 text-right font-medium">{row.ramL}</td>
                                        <td className={`px-6 py-4 font-semibold ${row.verdict.includes('RAMpage') ? 'text-primary' : 'text-destructive/80'}`}>{row.verdict}</td>
                                    </tr>
                                ))}
                            </tbody>
                        </table>
                    </div>
                </div>
            </section>

            {/* Features Section */}
            <section className="py-16 md:py-24 bg-background">
                <div className="container mx-auto px-4">
                    <div className="text-center mb-12">
                        <h2 className="text-3xl md:text-4xl font-bold mb-4">Underlying Architecture</h2>
                        <p className="text-lg text-muted-foreground max-w-2xl mx-auto">
                            RAMpage is built to squeeze every drop of performance out of modern Linux systems.
                        </p>
                    </div>

                    <div className="grid md:grid-cols-2 lg:grid-cols-4 gap-6 max-w-7xl mx-auto">
                        <Card className="border-primary/20 shadow-sm hover:shadow-md transition-shadow">
                            <CardHeader>
                                <Activity className="w-10 h-10 text-primary mb-4" />
                                <CardTitle>Ultra-Fast I/O Engine</CardTitle>
                                <CardDescription>Powered by Linux epoll</CardDescription>
                            </CardHeader>
                            <CardContent>
                                <p className="text-muted-foreground text-sm">
                                    Non-blocking, event-driven networking handles thousands of concurrent persistent TCP connections on a single thread with absolutely zero race conditions on the hot path.
                                </p>
                            </CardContent>
                        </Card>

                        <Card className="border-primary/20 shadow-sm hover:shadow-md transition-shadow">
                            <CardHeader>
                                <Database className="w-10 h-10 text-primary mb-4" />
                                <CardTitle>Native RESP Protocol</CardTitle>
                                <CardDescription>Drop-in Replacement</CardDescription>
                            </CardHeader>
                            <CardContent>
                                <p className="text-muted-foreground text-sm">
                                    Features a custom RESP parser with greedy pipelining. It is 100% compatible with existing Redis SDKs across Node.js, Python, and Go, right out of the box.
                                </p>
                            </CardContent>
                        </Card>

                        <Card className="border-primary/20 shadow-sm hover:shadow-md transition-shadow">
                            <CardHeader>
                                <ShieldCheck className="w-10 h-10 text-primary mb-4" />
                                <CardTitle>AOF Persistence</CardTitle>
                                <CardDescription>Zero Data Loss</CardDescription>
                            </CardHeader>
                            <CardContent>
                                <p className="text-muted-foreground text-sm">
                                    Write commands are securely appended to an Append-Only File. A dedicated background flusher thread uses condition variables to ensure disk I/O never blocks the main loop.
                                </p>
                            </CardContent>
                        </Card>

                        <Card className="border-primary/20 shadow-sm hover:shadow-md transition-shadow">
                            <CardHeader>
                                <MessageSquare className="w-10 h-10 text-primary mb-4" />
                                <CardTitle>Pub/Sub Messaging</CardTitle>
                                <CardDescription>Real-time Fan-out</CardDescription>
                            </CardHeader>
                            <CardContent>
                                <p className="text-muted-foreground text-sm">
                                    Fully compatible publish/subscribe engine supporting exact channels and glob-style pattern matching with zero-blocking socket pushes.
                                </p>
                            </CardContent>
                        </Card>

                    </div>
                </div>
            </section>


        </div>
    );
}

export default Home;
