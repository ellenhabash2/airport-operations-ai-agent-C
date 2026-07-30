import { render, screen } from "@testing-library/react";
import { describe, expect, it } from "vitest";
import MarkdownAnswer from "./MarkdownAnswer";

describe("MarkdownAnswer", () => {
  it("renders markdown and safe external links", () => {
    render(<MarkdownAnswer>{"## Update\n\n- **Flight** ready\n\n[Airport](https://example.com)"}</MarkdownAnswer>);
    expect(screen.getByRole("heading", { name: "Update" })).toBeVisible();
    expect(screen.getByText("Flight").tagName).toBe("STRONG");
    expect(screen.getByRole("link", { name: "Airport" })).toHaveAttribute("rel", "noopener noreferrer");
  });

  it("does not render raw HTML", () => {
    const { container } = render(<MarkdownAnswer>{"Hello <script>alert('x')</script> world"}</MarkdownAnswer>);
    expect(container.querySelector("script")).not.toBeInTheDocument();
  });

  it("renders paragraphs, ordered lists, tables, emphasis, and code", () => {
    const { container } = render(<MarkdownAnswer>{"Paragraph\n\n1. First\n2. Second\n\n| Flight | Gate |\n| --- | --- |\n| **SB2101** | *A03* |\n\nUse `status`:\n\n```json\n{\"ok\":true}\n```"}</MarkdownAnswer>);
    expect(screen.getByText("Paragraph").tagName).toBe("P");
    expect(screen.getByText("First").closest("ol")).toBeInTheDocument();
    expect(screen.getByRole("table")).toBeVisible();
    expect(screen.getByText("SB2101").tagName).toBe("STRONG");
    expect(screen.getByText("A03").tagName).toBe("EM");
    expect(container.querySelector("pre code")).toHaveTextContent('{"ok":true}');
  });

  it("removes script-like and relative link destinations", () => {
    render(<MarkdownAnswer>{"[bad](javascript:alert(1)) [relative](/admin)"}</MarkdownAnswer>);
    expect(screen.queryByRole("link", { name: "bad" })).not.toBeInTheDocument();
    expect(screen.queryByRole("link", { name: "relative" })).not.toBeInTheDocument();
    expect(screen.getByText("bad")).toBeVisible();
  });
});
