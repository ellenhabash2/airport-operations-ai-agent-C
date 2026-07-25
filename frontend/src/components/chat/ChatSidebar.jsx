import { Plane, Plus, ArrowLeft, MessageSquare } from "lucide-react";
import { useNavigate } from "react-router-dom";

const conversations = [
    {
        id: 1,
        title: "Runway 27 maintenance",
        date: "Today",
    },
    {
        id: 2,
        title: "Delayed flights today",
        date: "Yesterday",
    },
    {
        id: 3,
        title: "Weather at TLV",
        date: "Jul 22",
    },
];

export default function ChatSidebar() {

    const navigate = useNavigate();
    const activeConversationId = 1;

    return (

        <aside className="chat-sidebar">

            <div>

                <div className="sidebar-logo">

                    <Plane size={34} />

                    <div>

                        <h2>AeroMind</h2>

                        <span>Airport Operations AI</span>

                    </div>

                </div>

                <button className="new-chat-btn">

                    <Plus size={18} />

                    New Chat

                </button>

                <h4 className="sidebar-title">
                    Recent Conversations
                </h4>

                <div className="conversation-list">

                    {conversations.map((conversation) => (

                        <button
                            key={conversation.id}
                            className={`conversation-item ${
                                activeConversationId === conversation.id ? "active" : ""
                            }`}
                        >

                            <MessageSquare size={18} />

                            <div>

                                <p>{conversation.title}</p>

                                <span>{conversation.date}</span>

                            </div>

                        </button>

                    ))}

                </div>

            </div>

            <button
                className="back-btn"
                onClick={() => navigate("/overview")}
            >

                <ArrowLeft size={18} />

                Back to Overview

            </button>

        </aside>

    );
}