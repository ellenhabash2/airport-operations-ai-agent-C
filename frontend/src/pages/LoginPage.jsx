import { useState } from "react";
import { useNavigate, Link } from "react-router-dom";
import { login } from "../services/authService";
import "../styles/login.css";
import Brand from "../components/Brand";
import { useEffect } from "react";
import api from "../services/api";

function LoginPage() {
    const navigate = useNavigate();

    const [email, setEmail] = useState("");
    const [password, setPassword] = useState("");

    const [loading, setLoading] = useState(false);
    const [error, setError] = useState("");

    useEffect(() => {
        const verifyExistingSession = async () => {
            const token = localStorage.getItem("token");

            if (!token) {
                return;
            }

            try {
                await api.get("/auth/me");
                navigate("/overview", { replace: true });
            } catch (err) {
                if (err.response?.status === 401) {
                    localStorage.removeItem("token");
                    localStorage.removeItem("user");
                }
            }
        };

        verifyExistingSession();
    }, [navigate]);

    async function handleLogin(e) {
        e.preventDefault();
        const trimmedEmail = email.trim();
        if (!trimmedEmail) {
        alert("Please enter your email.");
        return;
        }

        if (!password) {
            alert("Please enter your password.");
            return;
        }

        setLoading(true);
        setError("");

        try {
            const data = await login(trimmedEmail, password);

            localStorage.setItem("token", data.token);
            localStorage.setItem("user", JSON.stringify(data.user));

            navigate("/overview");
        }
        catch (err) {
            setError(
                err.response?.data?.error ||
                "Login failed."
            );
        }
        finally {
            setLoading(false);
        }
    }

    return (
    <div className="login-page">

        <div className="login-card">

            <div className="login-brand">
                    <Brand />
            </div>  

            <div className="login-title">

                <h2>Welcome Back</h2>

                <p>
                    Sign in to your account
                </p>

            </div>

            <form onSubmit={handleLogin}>

                <div className="form-group">

                    <label className="form-label">
                        Email
                    </label>

                    <input
                        className="form-input"
                        type="email"
                        placeholder="Enter your email"
                        value={email}
                        onChange={(e) => {
                            setEmail(e.target.value);
                            setError("");
                        }}
                        disabled={loading}
                    />

                </div>

                <div className="form-group">

                    <label className="form-label">
                        Password
                    </label>

                    <input
                        className="form-input"
                        type="password"
                        placeholder="Enter your password"
                        value={password}
                        onChange={(e) => {
                            setPassword(e.target.value);
                            setError("");
                        }}
                        disabled={loading}
                    />

                </div>

                {error && (
                    <div className="form-error">
                        {error}
                    </div>
                )}

                <button
                    className="button button-primary button-full-width login-button"
                    type="submit"
                    disabled={loading}
                >
                    {loading ? "Logging in..." : "Login"}
                </button>

            </form>

            <div className="login-footer">

                <p>

                    Don't have an account?{" "}

                    <Link to="/register">
                        Register
                    </Link>

                </p>

            </div>

        </div>

    </div>
);
}

export default LoginPage;