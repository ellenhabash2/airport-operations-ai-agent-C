import ChatSidebar from "../components/chat/ChatSidebar";
import ChatHeader from "../components/chat/ChatHeader";
import ChatMessages from "../components/chat/ChatMessages";
import ChatInput from "../components/chat/ChatInput";
import SuggestedQuestions from "../components/chat/SuggestedQuestions";

import "../styles/chat.css";

export default function ChatPage() {
    return (
        <div className="chat-page">

            <ChatSidebar />

            <div className="chat-content">

                <div className="chat-main">

                    <ChatHeader />

                    <ChatMessages />

                    <ChatInput />

                </div>

                <SuggestedQuestions />

            </div>

        </div>
    );
}