import { SendHorizontal } from "lucide-react";
import { useState } from "react";

export default function ChatInput() {
    const [message, setMessage] = useState("");

    function handleSend() {

    if (!message.trim()) {
        return;
    }

    console.log(message);

    setMessage("");
    }

    return (

        <div className="chat-input-container">

            <div className="chat-input-box">

                <input
                    type="text"
                    placeholder="Ask anything about airport operations..."
                    value={message}
                    onChange={(e) => setMessage(e.target.value)}
                    onKeyDown={(e) => {
                        if (e.key === "Enter") {
                            handleSend();
                        }
                    }}
                />

                <button className="send-btn" onClick={handleSend}>

                    <SendHorizontal size={20} />

                </button>

            </div>

        </div>

    );

}