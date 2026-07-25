import { Bot } from "lucide-react";

export default function ChatMessages() {
    return (
        <div className="chat-messages">

            <div className="assistant-message">

                <div className="assistant-avatar">
                    <Bot size={22} />
                </div>

                <div className="message-card">

                    <p>
                        Hello! I'm <strong>AeroMind</strong>, your airport operations
                        assistant.
                    </p>

                    <p>
                        I can help you with flights, gates, runways,
                        weather, incidents and more.
                    </p>

                    <p>
                        What would you like to know?
                    </p>

                </div>

            </div>

        </div>
    );
}