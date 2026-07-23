import { useState } from "react";
import { Link, useNavigate } from "react-router-dom";

import { register } from "../services/authService";
import Brand from "../components/Brand";
import "../styles/login.css";

function RegisterPage() {
    const [username, setUsername] = useState("");
    const [email, setEmail] = useState("");
    const [password, setPassword] = useState("");

    const [loading, setLoading] = useState(false);
    const [error, setError] = useState("");

    const navigate = useNavigate(); 
    
    async function handleRegister(e) {
      e.preventDefault();
      
      const trimmedUsername = username.trim();
      const trimmedEmail = email.trim();

      if (!trimmedUsername) {
          alert("Please enter your username.");
          return;
      }

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
          await register(trimmedUsername, trimmedEmail, password);

          navigate("/");
      }
      catch (err) {
          setError(
              err.response?.data?.error ||
              "Registration failed."
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

                    <h2>Create Account</h2>

                    <p>
                        Create your AeroMind account
                    </p>

                </div>

                <form onSubmit={handleRegister}>

                    <div className="form-group">

                        <label className="form-label">
                            Username
                        </label>

                        <input
                            className="form-input"
                            type="text"
                            placeholder="Enter your username"
                            value={username}
                            onChange={(e) => {
                                setUsername(e.target.value);
                                setError("");
                            }}
                            disabled={loading}
                        />

                    </div>

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
                            placeholder="Create a password"
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
                        {loading ? "Creating account..." : "Register"}
                    </button>

                </form>

                <div className="login-footer">

                    <p>

                        Already have an account?{" "}

                        <Link to="/">
                            Login
                        </Link>

                    </p>

                </div>

            </div>

        </div>
    );
}

export default RegisterPage;