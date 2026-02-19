import { defineConfig } from 'vitepress'
import { endpointPlugin } from './plugins/endpoint.js'

const docsBase = process.env.DOCS_BASE || '/'

export default defineConfig({
  title: 'OSRM API Documentation',
  description: 'The Open Source Routing Machine API documentation',
  base: docsBase,
  ignoreDeadLinks: true,

  themeConfig: {
    nav: [
      { text: 'Home', link: '/' },
      { text: 'HTTP API', link: '/http' },
      { text: 'Node.js API', link: '/nodejs/api' }
    ],

    sidebar: [
      {
        text: 'Getting Started',
        items: [
          { text: 'Introduction', link: '/' },
          { text: 'Developing', link: '/developing' },
          { text: 'Testing', link: '/testing' },
          { text: 'Releasing', link: '/releasing' }
        ]
      },
      {
        text: 'API Documentation',
        items: [
          { text: 'HTTP API', link: '/http' },
          { text: 'Node.js API', link: '/nodejs/api' }
        ]
      },
      {
        text: 'Configuration',
        items: [
          { text: 'Profiles', link: '/profiles' },
          { text: 'Routed Service', link: '/routed' }
        ]
      },
      {
        text: 'Testing',
        items: [
          { text: 'Cucumber Tests', link: '/cucumber' }
        ]
      },
      {
        text: 'Platform Specific',
        items: [
          { text: 'Windows Dependencies', link: '/windows-deps' }
        ]
      }
    ],

    socialLinks: [
      { icon: 'github', link: 'https://github.com/Project-OSRM/osrm-backend' }
    ],

    search: {
      provider: 'local'
    },

    editLink: {
      pattern: 'https://github.com/Project-OSRM/osrm-backend/edit/master/docs/:path'
    }
  },

  markdown: {
    theme: {
      light: 'github-light',
      dark: 'github-dark'
    },
    config: (md) => {
      md.use(endpointPlugin)
    }
  }
})
