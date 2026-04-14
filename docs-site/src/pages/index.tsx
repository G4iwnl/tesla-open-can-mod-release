import type {ReactNode} from 'react';
import clsx from 'clsx';
import Link from '@docusaurus/Link';
import useDocusaurusContext from '@docusaurus/useDocusaurusContext';
import Layout from '@theme/Layout';
import Heading from '@theme/Heading';

import styles from './index.module.css';

function HomepageHeader() {
  const {siteConfig} = useDocusaurusContext();
  return (
    <header className={clsx('hero hero--primary', styles.heroBanner)}>
      <div className="container">
        <Heading as="h1" className="hero__title">
          {siteConfig.title}
        </Heading>
        <p className="hero__subtitle">{siteConfig.tagline}</p>
        <div className={styles.buttons}>
          <Link
            className="button button--secondary button--lg"
            to="/docs/intro">
            Get Started
          </Link>
          <Link
            className="button button--secondary button--lg"
            href="https://gitlab.com/tesla-open-can-mod/tesla-open-can-mod">
            View Source
          </Link>
        </div>
      </div>
    </header>
  );
}

const features = [
  {
    title: 'FSD Activation',
    description:
      'Enable Full Self-Driving at the CAN bus level. Supports HW3, HW4, and Legacy vehicles with dynamic speed profiles.',
  },
  {
    title: 'Open Source',
    description:
      'Fully transparent, GPL-3.0 licensed. No black boxes — read, audit, and contribute to every line of code.',
  },
  {
    title: 'Multi-Board Support',
    description:
      'Works with Feather RP2040 CAN, Feather M4 CAN Express, ESP32 with TWAI, and M5Stack Atomic CAN Base.',
  },
  {
    title: 'Actually Smart Summon',
    description:
      'Enables ASS (Actually Smart Summon) on HW4 vehicles without EU regulatory restrictions.',
  },
  {
    title: 'Enhanced Autopilot',
    description:
      'Adds an HW4 feature bundle for summon support and the existing traffic-light / stop-sign gated Autopilot path.',
  },
  {
    title: 'Affordable',
    description:
      'Hardware costs around 20 EUR. Some sellers charge 500 EUR for the same thing — this project exists so nobody has to overpay.',
  },
  {
    title: 'Tested & CI-backed',
    description:
      'Comprehensive unit tests with Unity framework. GitLab CI validates every commit across all board variants.',
  },
];

function FeatureSection() {
  return (
    <section className={styles.features}>
      <div className="container">
        <div className="row">
          {features.map((feature, idx) => (
            <div key={idx} className={clsx('col col--4', styles.featureItem)}>
              <div className="padding-horiz--md padding-vert--md">
                <Heading as="h3">{feature.title}</Heading>
                <p>{feature.description}</p>
              </div>
            </div>
          ))}
        </div>
      </div>
    </section>
  );
}

function DisclaimerBanner() {
  return (
    <section className="container" style={{marginTop: '2rem', marginBottom: '2rem'}}>
      <div className="disclaimer-banner">
        <strong>Warning:</strong> This project is for testing and educational purposes only.
        Sending incorrect CAN bus messages to your vehicle can cause unexpected behavior,
        disable safety-critical systems, or permanently damage electronic components.
        You must have an active FSD package (purchased or subscribed).
        <br />
        <Link to="/docs/legal/disclaimer">Read full disclaimer</Link>
      </div>
    </section>
  );
}

function SupportedHardware() {
  const boards = [
    {name: 'Feather RP2040 CAN', interface: 'MCP2515 over SPI', status: 'Tested'},
    {name: 'Feather M4 CAN Express', interface: 'Native MCAN (ATSAME51)', status: 'Tested'},
    {name: 'ESP32 + CAN Transceiver', interface: 'Native TWAI', status: 'Tested'},
    {name: 'M5Stack Atomic CAN Base', interface: 'CA-IS3050G over TWAI', status: 'Tested'},
    {name: 'M5Stack AtomS3 CAN Base', interface: 'CA-IS3050G over TWAI', status: 'Build target'},
  ];

  return (
    <section className="container" style={{marginBottom: '3rem'}}>
      <Heading as="h2" className="text--center" style={{marginBottom: '1.5rem'}}>
        Supported Boards
      </Heading>
      <div className="feature-grid">
        {boards.map((board, idx) => (
          <div key={idx} className="feature-card">
            <Heading as="h3">{board.name}</Heading>
            <p><strong>Interface:</strong> {board.interface}</p>
            <p><strong>Status:</strong> {board.status}</p>
          </div>
        ))}
      </div>
    </section>
  );
}

export default function Home(): ReactNode {
  return (
    <Layout
      title="Open-source CAN bus mod for Tesla"
      description="Open-source CAN bus modification tool for Tesla vehicles. FSD activation, enhanced autopilot, nag suppression, speed profiles, and more.">
      <HomepageHeader />
      <main>
        <DisclaimerBanner />
        <FeatureSection />
        <SupportedHardware />
      </main>
    </Layout>
  );
}
