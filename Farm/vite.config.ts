import { defineConfig } from 'vite';
import plugin from '@vitejs/plugin-react';

// https://vitejs.dev/config/
export default defineConfig({
    plugins: [plugin()],
    server: {
        port: 64735,
        // Allow tunnel/custom domains when testing on phone
        allowedHosts: true,
    }
})
