import api from "./api";

export async function login(email, password) {
    const response = await api.post("/auth/login", {
        email,
        password,
    });

    return response.data;
}


export async function register(username, email, password) {
    const response = await api.post("/auth/register", {
        username,
        email,
        password,
    });

    return response.data;
}