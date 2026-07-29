import { useCallback, useEffect, useRef, useState } from "react";
import { Link, useSearchParams } from "react-router-dom";
import { ArrowLeft, Bot, Menu, MessageSquarePlus, Send, Trash2, X } from "lucide-react";
import { api, ApiError } from "../api/client";
import { useAuth } from "../context/AuthContext";
import MarkdownAnswer from "../components/chat/MarkdownAnswer";
import StructuredAnswerRenderer from "../components/structured-answers/StructuredAnswerRenderer";
import ToolExecutionTimeline from "../components/tool-execution/ToolExecutionTimeline";
import FlightDetailsDrawer from "../components/flight-details/FlightDetailsDrawer";
import ConfirmDialog from "../components/common/ConfirmDialog";
import { parseAgentAnswer } from "../types/api";
import type { AgentAnswer, AgentPresentation, ConversationListResponse, ConversationMessagesResponse, ConversationSummary, Flight, ToolExecution } from "../types/api";

interface ChatTurn { key: string; role: "user" | "assistant"; text: string; toolExecutions?: ToolExecution[]; presentation?: AgentPresentation | null; createdAt?: string | null; }

const SUGGESTIONS = [
  "Which flights are currently delayed?", "Show all active incidents.", "Find an available gate.",
  "What is the status of flight SB2101?", "Which runway is currently closed?", "What is the latest weather report?",
];

function friendlyError(error: unknown) {
  if (error instanceof ApiError) {
    if (error.status === 401) return "Your session has expired. Please sign in again.";
    if (error.status === 503) return "The AI service is temporarily unavailable.";
    if (error.status >= 500 || error.status === 0) return "The request could not be completed.";
    return error.message;
  }
  return "The request could not be completed.";
}

export default function ChatPage() {
  const { user } = useAuth();
  const [searchParams] = useSearchParams();
  const [conversations, setConversations] = useState<ConversationSummary[]>([]);
  const [conversationId, setConversationId] = useState<number | null>(null);
  const [turns, setTurns] = useState<ChatTurn[]>([]);
  const [draft, setDraft] = useState(() => searchParams.get("prompt") ?? "");
  const [sending, setSending] = useState(false);
  const [error, setError] = useState<string | null>(null);
  const [sidebarOpen, setSidebarOpen] = useState(false);
  const [selectedFlight, setSelectedFlight] = useState<Flight | null>(null);
  const [pendingDelete, setPendingDelete] = useState<ConversationSummary | null>(null);
  const [deleting, setDeleting] = useState(false);
  const [deleteError, setDeleteError] = useState<string | null>(null);
  const endRef = useRef<HTMLDivElement>(null);
  const inputRef = useRef<HTMLTextAreaElement>(null);

  const loadConversations = useCallback(async () => {
    try { setConversations((await api.get<ConversationListResponse>("/agent/history")).conversations); } catch { /* Secondary navigation remains optional. */ }
  }, []);
  useEffect(() => { const timer = window.setTimeout(() => void loadConversations(), 0); return () => clearTimeout(timer); }, [loadConversations]);
  useEffect(() => { endRef.current?.scrollIntoView({ behavior: "smooth" }); }, [turns, sending]);

  function startNewConversation() { setConversationId(null); setTurns([]); setError(null); setSidebarOpen(false); inputRef.current?.focus(); }
  async function openConversation(id: number) {
    setError(null);
    try {
      const result = await api.get<ConversationMessagesResponse>(`/agent/conversations/${id}/messages`);
      setTurns(result.messages.filter((message) => message.content).map((message) => ({ key: `stored-${message.id}`, role: message.role, text: message.content as string, toolExecutions: message.tool_executions ?? [], presentation: message.presentation, createdAt: message.created_at })));
      setConversationId(id); setSidebarOpen(false);
    } catch (caught) { setError(friendlyError(caught)); }
  }
  async function deleteConversation(id: number) {
    setDeleting(true); setDeleteError(null);
    try { await api.delete(`/agent/conversations/${id}`); setConversations((items) => items.filter((item) => item.id !== id)); if (conversationId === id) startNewConversation(); setPendingDelete(null); }
    catch (caught) { setDeleteError(friendlyError(caught)); }
    finally { setDeleting(false); }
  }
  async function sendMessage(message: string) {
    const trimmed = message.trim(); if (!trimmed || sending) return;
    setDraft(""); setError(null); setTurns((items) => [...items, { key: `user-${Date.now()}`, role: "user", text: trimmed }]); setSending(true);
    try {
      const payload = await api.post<unknown>("/agent/query", { query: trimmed, conversation_id: conversationId });
      const result: AgentAnswer = parseAgentAnswer(payload);
      setTurns((items) => [...items, { key: `agent-${Date.now()}`, role: "assistant", text: result.answer, toolExecutions: result.tool_executions, presentation: result.presentation }]);
      setConversationId(result.conversation_id); void loadConversations();
    } catch (caught) { setDraft(trimmed); setError(friendlyError(caught)); }
    finally { setSending(false); }
  }

  const activeTitle = conversations.find((item) => item.id === conversationId)?.title ?? "New conversation";
  const sidebar = <aside aria-label="Conversation history" className="flex h-full w-[285px] shrink-0 flex-col border-r border-white/10 bg-surface/95 lg:bg-black/15">
    <div className="border-b border-white/10 px-4 py-4"><div className="mb-4 flex items-center justify-between"><Link to="/" className="flex items-center gap-2 text-xs text-muted hover:text-white"><ArrowLeft className="h-3.5 w-3.5" />Back to overview</Link><button type="button" aria-label="Close conversations" onClick={() => setSidebarOpen(false)} className="text-muted lg:hidden"><X className="h-5 w-5" /></button></div>
      <button type="button" onClick={startNewConversation} className="flex w-full items-center justify-center gap-2 rounded-xl bg-gradient-to-r from-accent-strong via-accent to-cyan py-2.5 text-sm font-semibold text-white"><MessageSquarePlus className="h-4 w-4" />New conversation</button></div>
    <div className="flex-1 space-y-1 overflow-y-auto px-3 py-4">{conversations.length === 0 ? <p className="px-2 py-6 text-center text-xs text-muted">Your conversations will appear here.</p> : conversations.map((conversation) => <div key={conversation.id} className={`group flex items-center gap-2 rounded-xl border px-3 py-2.5 ${conversationId === conversation.id ? "border-accent/15 bg-accent-strong/20" : "border-transparent hover:bg-white/[0.045]"}`}><button type="button" onClick={() => void openConversation(conversation.id)} className="min-w-0 flex-1 text-left"><p className="truncate text-sm text-white">{conversation.title}</p><p className="mt-0.5 text-xs text-muted">{conversation.message_count} messages</p></button><button type="button" aria-label={`Delete ${conversation.title}`} onClick={() => { setDeleteError(null); setPendingDelete(conversation); }} className="shrink-0 p-1 text-muted hover:text-alert"><Trash2 className="h-3.5 w-3.5" /></button></div>)}</div>
  </aside>;

  return <div className="min-h-screen p-0 sm:px-5 sm:py-5"><div className="glass-panel mx-auto flex h-[100dvh] max-w-[1540px] overflow-hidden sm:h-[calc(100vh-2.5rem)] sm:rounded-[28px]">
    <div className="hidden lg:block">{sidebar}</div>
    {sidebarOpen && <div className="fixed inset-0 z-40 lg:hidden"><button aria-label="Close conversations" className="absolute inset-0 bg-black/65" onClick={() => setSidebarOpen(false)} /><div className="relative h-full">{sidebar}</div></div>}
    <main className="flex min-w-0 flex-1 flex-col">
      <header className="flex items-center justify-between gap-3 border-b border-white/10 px-4 py-3 sm:px-6 sm:py-4"><div className="flex min-w-0 items-center gap-3"><button type="button" aria-label="Open conversations" aria-expanded={sidebarOpen} onClick={() => setSidebarOpen(true)} className="rounded-lg p-2 text-muted lg:hidden"><Menu className="h-5 w-5" /></button><span className="flex h-10 w-10 shrink-0 items-center justify-center rounded-xl bg-gradient-to-br from-cyan to-accent text-white"><Bot className="h-5 w-5" /></span><div className="min-w-0"><h1 className="truncate font-semibold text-white">{activeTitle}</h1><p className="flex items-center gap-1.5 truncate text-xs text-muted"><span className="h-1.5 w-1.5 rounded-full bg-clear" />AeroMind · Ready for operational questions</p></div></div><Link to="/" className="text-xs text-muted hover:text-white lg:hidden">Overview</Link></header>
      <div className="flex-1 overflow-y-auto overflow-x-hidden px-4 py-6 sm:px-6">
        {turns.length === 0 ? <section className="mx-auto max-w-2xl py-5 text-center sm:py-10"><span className="mx-auto flex h-14 w-14 items-center justify-center rounded-2xl bg-cyan/10 text-cyan"><Bot className="h-7 w-7" /></span><h2 className="mt-5 text-2xl font-bold text-white">Welcome to AeroMind{user?.username ? `, ${user.username}` : ""}</h2><p className="mx-auto mt-2 max-w-lg text-sm text-muted">Your airport operations assistant for flights, gates, runways, incidents, and weather.</p><div className="mt-7 grid gap-3 sm:grid-cols-2">{SUGGESTIONS.map((suggestion) => <button key={suggestion} type="button" onClick={() => { setDraft(suggestion); inputRef.current?.focus(); }} className="rounded-2xl border border-white/10 bg-white/[0.04] p-4 text-left text-sm text-muted-light hover:border-cyan/30 hover:text-white">{suggestion}</button>)}</div></section> : <div className="mx-auto max-w-3xl space-y-6">{turns.map((turn) => turn.role === "user" ? <article key={turn.key} aria-label="You" className="flex justify-end"><p className="max-w-[85%] whitespace-pre-wrap break-words rounded-2xl rounded-br-md bg-gradient-to-r from-accent-strong to-accent px-4 py-3 text-sm text-white sm:max-w-[75%]">{turn.text}</p></article> : <article key={turn.key} aria-label="AeroMind" className="flex min-w-0 gap-3"><span className="mt-1 flex h-8 w-8 shrink-0 items-center justify-center rounded-xl bg-gradient-to-br from-cyan to-accent"><Bot className="h-4 w-4" /></span><div className="min-w-0 flex-1 space-y-4"><ToolExecutionTimeline executions={turn.toolExecutions} /><MarkdownAnswer>{turn.text}</MarkdownAnswer><StructuredAnswerRenderer presentation={turn.presentation} onOpenFlight={setSelectedFlight} onPrompt={(prompt) => { setDraft(prompt); inputRef.current?.focus(); }} /></div></article>)}
          {sending && <div role="status" aria-live="polite" className="flex gap-3"><span className="flex h-8 w-8 items-center justify-center rounded-xl bg-gradient-to-br from-cyan to-accent"><Bot className="h-4 w-4" /></span><div className="rounded-2xl border border-white/10 bg-white/[0.04] px-4 py-3"><p className="text-sm text-muted-light">AeroMind is analyzing airport operations…</p><p className="mt-1 text-xs text-muted">Preparing response</p></div></div>}<div ref={endRef} /></div>}
      </div>
      <footer className="border-t border-white/10 bg-surface/70 px-4 py-3 backdrop-blur-xl sm:px-6 sm:py-4"><div className="mx-auto max-w-3xl">{error && <p role="alert" className="mb-3 rounded-xl border border-alert/25 bg-alert/10 px-3.5 py-2.5 text-sm text-alert">{error}</p>}<label htmlFor="chat-message" className="sr-only">Message AeroMind</label><div className="flex items-end gap-3"><textarea id="chat-message" ref={inputRef} value={draft} onChange={(event) => setDraft(event.target.value)} onKeyDown={(event) => { if (event.key === "Enter" && !event.shiftKey) { event.preventDefault(); void sendMessage(draft); } }} rows={1} placeholder="Ask about delayed flights, gates or incidents…" className="max-h-40 min-h-12 min-w-0 flex-1 resize-none rounded-xl border border-white/10 bg-black/15 px-4 py-3 text-sm text-white outline-none placeholder:text-muted/55 focus:border-cyan/60" /><button type="button" onClick={() => void sendMessage(draft)} disabled={sending || !draft.trim()} aria-label="Send message" className="flex h-12 w-12 shrink-0 items-center justify-center rounded-xl bg-gradient-to-r from-accent-strong via-accent to-cyan text-white disabled:opacity-40"><Send className="h-4 w-4" /></button></div><p className="mt-2 text-center text-xs text-muted">AeroMind can inspect flights, gates, runways, incidents, and weather. Enter to send · Shift+Enter for a new line.</p></div></footer>
    </main>
    <ConfirmDialog open={pendingDelete !== null} title="Delete conversation?" description={`Delete “${pendingDelete?.title ?? "this conversation"}”? This cannot be undone.`} confirming={deleting} error={deleteError} onCancel={() => { if (!deleting) setPendingDelete(null); }} onConfirm={() => { if (pendingDelete) void deleteConversation(pendingDelete.id); }} />
    {selectedFlight && <FlightDetailsDrawer flightId={selectedFlight.id} initialFlight={selectedFlight} onClose={() => setSelectedFlight(null)} />}
  </div></div>;
}
