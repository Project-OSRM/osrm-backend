/**
 * VitePress markdown-it plugin for rendering API endpoints
 * Transforms ```endpoint blocks into styled HTML
 */

function highlightTokens(str) {
  return str.replace(/{[\w_]+}/g, (match) => `<span class="strong">${match}</span>`)
}

export function endpointPlugin(md) {
  const fence = md.renderer.rules.fence.bind(md.renderer.rules)

  md.renderer.rules.fence = (tokens, idx, options, env, slf) => {
    const token = tokens[idx]
    const info = token.info.trim()

    if (info === 'endpoint') {
      const content = token.content.trim()
      const parts = content.split(/\s+/)

      if (parts.length >= 2) {
        const method = parts[0]
        const url = parts.slice(1).join(' ')

        return `<div class="endpoint">
  <div class="endpoint-method">${method}</div>
  <div class="endpoint-url">${highlightTokens(url)}</div>
</div>`
      }
    }

    return fence(tokens, idx, options, env, slf)
  }
}
