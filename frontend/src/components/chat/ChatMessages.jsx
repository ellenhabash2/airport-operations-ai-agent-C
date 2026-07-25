import { Bot } from "lucide-react";

export default function ChatMessages({ messages, loading  }) {

    return (

        <div className="chat-messages">

            {messages.map((message) => (

                <div
                    key={message.id}
                    className={`message-row ${message.sender}`}
                >

                    {message.sender === "assistant" && (
                        <div className="assistant-avatar">
                            <Bot size={22} />
                        </div>
                    )}

                    <div className="message-card">

                        {message.text.split("\n").map((line, index) => (
                            <p key={index}>{line}</p>
                        ))}

                    </div>

                </div>

            ))}
            {loading && (

                <div className="message-row assistant">

                    <div className="assistant-avatar">
                        <Bot size={22} />
                    </div>

                    <div className="message-card">
                        <p>AeroMind is thinking...</p>
                    </div>

                </div>

            )}

        </div>

    );

}