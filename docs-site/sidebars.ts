import type {SidebarsConfig} from '@docusaurus/plugin-content-docs';

const sidebars: SidebarsConfig = {
  docsSidebar: [
    'intro',
    {
      type: 'category',
      label: 'Getting Started',
      items: [
        'getting-started/hardware-selection',
        'getting-started/installation',
        'getting-started/installation-m4',
        'getting-started/fsd-subscription',
        'getting-started/wiring-legacy',
        'getting-started/firmware-flash',
        'getting-started/configuration',
      ],
    },
    {
      type: 'category',
      label: 'Features',
      items: [
        'features/fsd-activation',
        'features/nag-suppression',
        'features/autosteer-nag-killer',
        'features/speed-profiles',
        'features/enhanced-autopilot',
        'features/smart-summon',
        'features/emergency-vehicle-detection',
        'features/isa-speed-chime',
        'features/web-interface',
      ],
    },
    {
      type: 'category',
      label: 'Hardware',
      items: [
        'hardware/feather-rp2040',
        'hardware/feather-m4',
        'hardware/esp32',
        'hardware/feather-v2-featherwing',
        'hardware/m5stack',
        'hardware/lilygo-tcan485'
      ],
    },
    {
      type: 'category',
      label: 'Development',
      items: [
        'development/architecture',
        'development/contributing',
        'development/testing',
      ],
    },
    {
      type: 'category',
      label: 'Wiki',
      items: [
        'wiki/faq',
        'wiki/troubleshooting',
        'wiki/compatibility',
        'wiki/can-bus-basics',
      ],
    },
    {
      type: 'category',
      label: 'Legal',
      items: [
        'legal/disclaimer',
      ],
    },
    'changelog',
  ],
};

export default sidebars;
