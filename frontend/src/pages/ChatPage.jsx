import ChatSidebar from "../components/chat/ChatSidebar";
import ChatHeader from "../components/chat/ChatHeader";
import ChatMessages from "../components/chat/ChatMessages";
import ChatInput from "../components/chat/ChatInput";
import SuggestedQuestions from "../components/chat/SuggestedQuestions";
import { useState } from "react";
import api from "../services/api";

import "../styles/chat.css";

export default function ChatPage() {
    const [messages, setMessages] = useState([
        {
        id: 1,
        sender: "assistant",
        text: `Hello! I'm AeroMind, your airport operations assistant.

        I can help you with flights, gates, runways,
        weather, incidents and more.

        What would you like to know?`,
        },
        ]);

    const [message, setMessage] = useState("");
    const [loading, setLoading] = useState(false);
    const [conversationId, setConversationId] = useState(null);

    async function handleSend() {

        if (!message.trim()) {
            return;
        }

        const userQuery = message;

        const userMessage = {
            id: Date.now(),
            sender: "user",
            text: userQuery,
        };

        setMessages((prev) => [...prev, userMessage]);

        setLoading(true);

        setMessage("");

        try {

            const requestBody = {
                query: userQuery,
            };

            if (conversationId) {
                requestBody.conversation_id = conversationId;
            }

            const response = await api.post(
                "/agent/query",
                requestBody
            );

            if (response.data.conversation_id) {
                setConversationId(response.data.conversation_id);
            }

            const assistantMessage = {
                id: Date.now() + 1,
                sender: "assistant",
                text: response.data.answer,
            };

            setMessages((prev) => [
                ...prev,
                assistantMessage,
            ]);

        } catch (error) {

            console.error(error);

            let errorMessage =
                "Sorry, I couldn't process your request. Please try again.";

            const status = error.response?.status;

            if (status === 401) {
                errorMessage =
                    "Your session has expired. Please sign in again.";
            }
            else if (status === 502) {
                errorMessage =
                    "The AI service is temporarily unavailable. Please try again in a few moments.";
            }
            else if (status === 500) {
                errorMessage =
                    "An internal server error occurred. Please try again later.";
            }
            else if (!error.response) {
                errorMessage =
                    "Unable to connect to the server. Please check your internet connection.";
            }

            setMessages((prev) => [
                ...prev,
                {
                    id: Date.now() + 1,
                    sender: "assistant",
                    text: errorMessage,
                },
            ]);
        } finally {

            setLoading(false);

        }

    }

    return (
        <div className="chat-page">

            <ChatSidebar />

            <div className="chat-content">

                <div className="chat-main">

                    <ChatHeader />

                    <ChatMessages messages={messages} loading={loading}/>

                    <ChatInput message={message} setMessage={setMessage} handleSend={handleSend}/>

                </div>

                <SuggestedQuestions />

            </div>

        </div>
    );
}