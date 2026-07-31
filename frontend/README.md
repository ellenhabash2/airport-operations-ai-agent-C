Names:Roaa Zoabi, Marwa Abd Alrzaq, Ellen Habash
# AeroMind frontend

The frontend is a React 18 and Vite application for registration, login, airport operations overview, AI chat, and persistent conversation selection.

## Commands

```bash
npm ci
npm run dev -- --host 0.0.0.0
npm run lint
npm run test:run
npm run build
```

The development server uses `http://localhost:5173`. Vite proxies `/api` to the Drogon backend at `http://localhost:8848`; start the backend with Docker Compose first.

Authentication tokens and public user information are stored in browser local storage. The selected chat conversation is stored in session storage. Gemini credentials never enter the frontend.
