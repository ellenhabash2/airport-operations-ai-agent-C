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
});
