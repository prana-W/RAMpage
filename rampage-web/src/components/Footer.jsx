import { Link } from 'react-router-dom';
import { Github, Database } from 'lucide-react';
import { Button } from './ui/button';

function Footer() {
    return (
        <footer className="border-t bg-muted/40 py-6 md:py-0">
            <div className="container mx-auto px-4 flex flex-col md:h-16 md:flex-row items-center justify-between gap-4">
                <div className="flex flex-col items-center gap-4 px-8 md:flex-row md:gap-2 md:px-0">
                    <Database className="h-5 w-5 text-primary" />
                    <p className="text-center text-sm leading-loose text-muted-foreground md:text-left">
                        Built by prana-w. The source code is available on{' '}
                        <a
                            href="https://github.com/prana-w/rampage"
                            target="_blank"
                            rel="noreferrer"
                            className="font-medium underline underline-offset-4"
                        >
                            GitHub
                        </a>
                        .
                    </p>
                </div>
                <div className="flex items-center space-x-2">
                     <Button variant="ghost" size="icon" asChild>
                        <a href="https://github.com/prana-w/rampage" target="_blank" rel="noreferrer">
                            <Github className="h-5 w-5 text-muted-foreground" />
                            <span className="sr-only">GitHub</span>
                        </a>
                    </Button>
                </div>
            </div>
        </footer>
    );
}

export default Footer;
