import { useEffect, useRef } from "react";

interface ConfirmDialogProps {
  open: boolean;
  title: string;
  description: string;
  confirming?: boolean;
  error?: string | null;
  onCancel: () => void;
  onConfirm: () => void;
}

export default function ConfirmDialog({ open, title, description, confirming = false, error, onCancel, onConfirm }: ConfirmDialogProps) {
  const cancelRef = useRef<HTMLButtonElement>(null);
  useEffect(() => {
    if (!open) return;
    cancelRef.current?.focus();
    const close = (event: KeyboardEvent) => { if (event.key === "Escape" && !confirming) onCancel(); };
    window.addEventListener("keydown", close);
    return () => window.removeEventListener("keydown", close);
  }, [confirming, onCancel, open]);
  if (!open) return null;
  return <div className="fixed inset-0 z-50 flex items-center justify-center p-4" role="presentation">
    <button aria-label="Close confirmation" className="absolute inset-0 bg-black/70" disabled={confirming} onClick={onCancel} />
    <section role="alertdialog" aria-modal="true" aria-labelledby="confirm-title" aria-describedby="confirm-description" className="relative w-full max-w-md rounded-2xl border border-white/10 bg-surface p-5 shadow-2xl">
      <h2 id="confirm-title" className="text-lg font-semibold text-white">{title}</h2>
      <p id="confirm-description" className="mt-2 text-sm text-muted">{description}</p>
      {error && <p role="alert" className="mt-3 text-sm text-alert">{error}</p>}
      <div className="mt-5 flex justify-end gap-2"><button ref={cancelRef} type="button" disabled={confirming} onClick={onCancel} className="rounded-lg border border-white/10 px-3 py-2 text-sm text-muted">Cancel</button><button type="button" disabled={confirming} onClick={onConfirm} className="rounded-lg bg-alert px-3 py-2 text-sm font-medium text-white disabled:opacity-50">{confirming ? "Deleting…" : "Delete conversation"}</button></div>
    </section>
  </div>;
}
