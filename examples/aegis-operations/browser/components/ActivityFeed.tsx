// @venom: browser
import React from "../jsx-runtime"; import type { Activity } from "../data/types";
interface ActivityFeedProps { activity:Activity[]; }
export default function ActivityFeed({activity}:ActivityFeedProps){ return <article className="panel activity-panel"><div className="panel-heading"><div><p className="eyebrow">Audit stream</p><h2>Recent activity</h2></div></div><div className="activity-list">{activity.slice(0,10).map(item=><div className="activity-item"><span className={`activity-mark ${item.tone}`}></span><div><strong>{item.actor}</strong> {item.action} <b>{item.target}</b><small>{item.time}</small></div></div>)}</div></article>; }
