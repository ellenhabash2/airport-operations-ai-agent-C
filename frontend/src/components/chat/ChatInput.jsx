import { SendHorizontal } from "lucide-react";

export default function ChatInput({message,setMessage,handleSend,}) {

  
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