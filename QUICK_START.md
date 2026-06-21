# AeroMind Quick Start Guide

## 📋 Prerequisites

- Docker & Docker Compose installed
- Ports 8848 and 5432 available on your machine

## 🚀 Get Started in 3 Steps

### Step 1: Build and Start Services

```bash
cd /workspaces/airport-operations-ai-agent-C
docker compose up --build
```

**Wait for output:**
```
aeromind_backend  | AeroMind Backend starting on port 8848...
aeromind_backend  | Database: aeromind @ postgres:5432
```

### Step 2: Verify Services Running

```bash
# Health check
curl http://localhost:8848/health

# Expected response:
# {"status":"ok","service":"AeroMind C++ Backend"}
```

### Step 3: Test API Endpoints

```bash
# Get all flights
curl http://localhost:8848/flights | jq .

# Get all gates
curl http://localhost:8848/gates | jq .

# Get all runways
curl http://localhost:8848/runways | jq .

# Get incidents
curl http://localhost:8848/incidents | jq .

# Get weather
curl http://localhost:8848/weather | jq .
```

## 📝 Common Commands

### Stop Services
```bash
docker compose down
```

### Stop and Clean Database (Reset)
```bash
docker compose down -v
```

### View Logs
```bash
docker compose logs -f backend
docker compose logs -f postgres
```

### Access PostgreSQL Directly
```bash
docker exec -it aeromind_postgres psql -U aeromind_user -d aeromind

# Common queries:
SELECT COUNT(*) FROM flights;
SELECT COUNT(*) FROM incidents;
SELECT * FROM runways;
```

## 🔍 Testing Endpoints

### Create an Incident
```bash
curl -X POST http://localhost:8848/incidents \
  -H "Content-Type: application/json" \
  -d '{
    "title": "Test Incident",
    "description": "This is a test incident",
    "severity": "LOW",
    "location": "Terminal 1"
  }' | jq .
```

### Report Weather
```bash
curl -X POST http://localhost:8848/weather \
  -H "Content-Type: application/json" \
  -d '{
    "condition": "Cloudy",
    "visibility_km": 9.5,
    "wind_speed_kmh": 12.0,
    "temperature_c": 18.5
  }' | jq .
```

### Get Flight Details
```bash
curl http://localhost:8848/flights/1 | jq .
```

## 📚 Documentation

- **Full README**: See [README.md](README.md)
- **API Documentation**: See [docs/API.md](docs/API.md) - All 13 endpoints
- **Database Schema**: See [docs/DATABASE.md](docs/DATABASE.md) - 14 tables, relationships
- **Database Queries**: See [sql/init.sql](sql/init.sql) and [sql/seed.sql](sql/seed.sql)

## 🏗️ Project Structure

```
airport-operations-ai-agent-C/
├── backend/                 # C++ Drogon backend
│   ├── controllers/        # HTTP endpoint handlers (8 controllers)
│   ├── repositories/       # Database access layer (5 repositories)
│   ├── database/           # Database connection management
│   ├── models/             # Data models (Phase 2)
│   ├── services/           # Business logic (Phase 2)
│   ├── tools/              # AI tools (Phase 3)
│   ├── agent/              # Agentic loop (Phase 3)
│   ├── security/           # Auth utilities (Phase 2)
│   ├── utils/              # Helper functions
│   └── main.cpp            # Entry point
├── sql/
│   ├── init.sql            # Database schema
│   └── seed.sql            # Seed data (30 flights, 5 airlines, etc.)
├── docs/
│   ├── API.md              # API endpoint documentation
│   └── DATABASE.md         # Database schema documentation
├── docker-compose.yml      # Docker Compose configuration
├── Dockerfile              # Multi-stage Docker build
├── CMakeLists.txt          # CMake build configuration
└── README.md               # Full project documentation
```

## 📊 Database Details

- **Engine**: PostgreSQL 16
- **Database**: aeromind
- **User**: aeromind_user
- **Password**: aeromind_password
- **Port**: 5432

### Seed Data Includes:
- 5 Airlines
- 25 Aircraft
- 3 Terminals
- 36 Gates
- 3 Runways
- 30 Flights
- 10 Crew Members
- 10 Weather Reports
- 10 Incidents

## 🔌 API Endpoints (13 Total)

### Core Endpoints
- `GET /health` - Service health check
- `GET /flights` - List all flights
- `GET /flights/{id}` - Get flight details
- `GET /gates` - List all gates
- `GET /runways` - List all runways

### Incident Management
- `GET /incidents` - List all incidents
- `POST /incidents` - Create incident

### Weather
- `GET /weather` - Get latest weather
- `POST /weather` - Report weather

### Placeholder Endpoints (Phase 2+)
- `POST /auth/register` - Authentication (Phase 2)
- `POST /auth/login` - Login (Phase 2)
- `POST /agent/query` - AI query (Phase 3)
- `GET /agent/history` - Chat history (Phase 3)

## 🚧 What's Not Implemented Yet (Planned Phases)

### Phase 2: Security & Auth
- JWT authentication
- Password hashing
- Authorization middleware

### Phase 3: AI Integration
- Gemini API integration
- AI function tools (8+)
- Agentic Loop

### Phase 4: Advanced Features
- WebSocket support
- Real-time updates
- Performance analytics

### Phase 5: Frontend
- React UI dashboard
- Chat interface

## 🐛 Troubleshooting

### Services won't start
```bash
# Check port availability
lsof -i :8848
lsof -i :5432

# Clean up old containers
docker compose down -v
docker system prune -a
```

### Database connection error
```bash
# Check database logs
docker compose logs postgres

# Wait a bit longer for database to initialize
docker compose down
sleep 5
docker compose up --build
```

### Port already in use
```bash
# Change ports in docker-compose.yml or use:
docker compose up -p aeromind_port --build
```

## 📞 Getting Help

- Check the full [README.md](README.md) for detailed information
- Review [docs/API.md](docs/API.md) for endpoint specifications
- Check [docs/DATABASE.md](docs/DATABASE.md) for database schema details
- Inspect logs: `docker compose logs -f backend`

## ✅ Quick Verification Checklist

- [ ] Docker & Docker Compose installed
- [ ] Ports 8848 and 5432 are free
- [ ] `docker compose up --build` completes successfully
- [ ] `curl http://localhost:8848/health` returns `{"status":"ok",...}`
- [ ] `curl http://localhost:8848/flights` returns flight data
- [ ] `docker exec -it aeromind_postgres psql -U aeromind_user -d aeromind` works

---

**You're all set! The AeroMind backend is ready for Phase 1 development.** 🎉
