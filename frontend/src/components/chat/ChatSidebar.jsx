import { Plane, Plus, ArrowLeft, MessageSquare } from "lucide-react";
import { useNavigate } from "react-router-dom";



export default function ChatSidebar({conversations,onNewChat, onConversationClick, selectedConversationId,}) {

    const navigate = useNavigate();
    

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

                <button className="new-chat-btn" onClick={onNewChat}>

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
                                selectedConversationId === conversation.id ? "active" : ""
                            }`}
                            onClick={() => onConversationClick(conversation.id)}
                        >

                            <MessageSquare size={18} />

                            <div>

                                <p>{conversation.title}</p>

                                <span>
                                    {new Date(conversation.created_at).toLocaleDateString("en-GB")}
                                </span>

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