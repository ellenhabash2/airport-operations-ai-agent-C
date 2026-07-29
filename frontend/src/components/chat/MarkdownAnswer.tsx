import ReactMarkdown from "react-markdown";
import remarkGfm from "remark-gfm";

export default function MarkdownAnswer({ children }: { children: string }) {
  return <div className="chat-markdown text-sm leading-6 text-muted-light">
    <ReactMarkdown
      remarkPlugins={[remarkGfm]}
      skipHtml
      urlTransform={(url) => /^https?:\/\//i.test(url) ? url : ""}
      components={{
        a: ({ children: label, ...props }) => <a {...props} target="_blank" rel="noopener noreferrer" className="text-cyan underline underline-offset-2">{label}</a>,
      }}
    >{children}</ReactMarkdown>
  </div>;
}
