import { Link } from 'react-router-dom';
import { Github, Database } from 'lucide-react';
import { Button } from './ui/button';
import { useTheme } from './theme-provider';

function Header() {
    const { theme } = useTheme();

    return (
        <header className="sticky top-0 z-50 w-full border-b bg-background/95 backdrop-blur supports-[backdrop-filter]:bg-background/60">
            <div className="container mx-auto px-4 h-14 flex items-center justify-between">
                <div className="flex items-center gap-6 md:gap-10">
                    <Link to="/" className="flex items-center space-x-2">
                        <Database className="h-6 w-6 text-primary" />
                        <span className="font-bold text-xl hidden sm:inline-block">RAMpage</span>
                    </Link>
                    <nav className="hidden md:flex gap-6">
                        <Link
                            to="/"
                            className="transition-colors hover:text-foreground/80 text-foreground/60 font-medium"
                        >
                            Home
                        </Link>
                        <Link
                            to="/commands"
                            className="transition-colors hover:text-foreground/80 text-foreground/60 font-medium"
                        >
                            Commands
                        </Link>
                        <Link
                            to="/about"
                            className="transition-colors hover:text-foreground/80 text-foreground/60 font-medium"
                        >
                            About
                        </Link>
                    </nav>
                </div>
                <div className="flex items-center space-x-2">
                    <Button variant="ghost" size="icon" asChild>
                        <a href="https://github.com/prana-w/rampage" target="_blank" rel="noreferrer">
                            <Github className="h-5 w-5" />
                            <span className="sr-only">GitHub</span>
                        </a>
                    </Button>
                </div>
            </div>
        </header>
    );
}

export default Header;
