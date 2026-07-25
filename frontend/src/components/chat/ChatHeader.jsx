import { Bot } from "lucide-react";

export default function ChatHeader() {
    return (
        <header className="chat-header">

            <div className="chat-header-info">

                <div className="chat-header-icon">
                    <Bot size={26} />
                </div>

                <div>

                    <h2>AI Assistant</h2>

                    <p>Your airport operations assistant</p>

                </div>

            </div>

        </header>
    );
}