---
layout: home

hero:
  name: OSRM
  text: API Documentation
  tagline: The Open Source Routing Machine - High performance routing engine for OpenStreetMap data
  actions:
    - theme: brand
      text: HTTP API
      link: /http
    - theme: alt
      text: Node.js API
      link: /nodejs/api
    - theme: alt
      text: View on GitHub
      link: https://github.com/Project-OSRM/osrm-backend

features:
  - icon: 🚗
    title: Route Service
    details: Find the fastest path between coordinates with support for alternative routes and turn-by-turn instructions.
  - icon: 📊
    title: Table Service
    details: Compute time and distance matrices between multiple locations for optimization problems.
  - icon: 🗺️
    title: Map Matching
    details: Match GPS traces to the road network with high accuracy using the Match service.
  - icon: 🎯
    title: Trip Planning
    details: Solve the traveling salesman problem with the Trip service for optimal route ordering.
  - icon: 📍
    title: Nearest Service
    details: Snap coordinates to the street network and find the nearest road segments.
  - icon: 🎨
    title: Tile Service
    details: Generate vector tiles for visualizing the road network and routing data.
---

## Getting Started

OSRM provides powerful routing services through both HTTP and Node.js APIs:

- **[HTTP API](./http.md)** - RESTful API for routing services
- **[Node.js API](./nodejs/api.md)** - Native Node.js bindings for embedded use

## Documentation

- [Developing](./developing.md) - Development guide
- [Profiles](./profiles.md) - Lua profile configuration
- [Testing](./testing.md) - Testing guidelines
- [Routed Service](./routed.md) - HTTP server configuration

## Resources

- [Project Website](https://project-osrm.org)
- [GitHub Repository](https://github.com/Project-OSRM/osrm-backend)
- [Issue Tracker](https://github.com/Project-OSRM/osrm-backend/issues)
